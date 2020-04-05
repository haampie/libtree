#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <vector>
#include <optional>
#include <string_view>
#include <functional>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>

#include <sys/auxv.h>

#include <cxxopts/cxxopts.hpp>
#include <termcolor/termcolor.hpp>
#include <cppglob/glob.hpp>

namespace fs = std::filesystem;

enum class deploy_t { EXECUTABLE, LIBRARY };

struct Elf {
    deploy_t type;
    fs::path abs_path;
    std::vector<fs::path> runpaths;
    std::vector<fs::path> rpaths;
    std::vector<fs::path> needed;
};

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    return result;
}

std::ostream &operator<<(std::ostream &os, Elf const &elf) {
    os << termcolor::on_green << (elf.type == deploy_t::EXECUTABLE ? "Executable" : "Shared library") << termcolor::reset << ' ';
    os << termcolor::green << elf.abs_path.string() << termcolor::reset << '\n';
    for (auto const &lib : elf.needed)
        os << "+ " << lib.string() << '\n';

    if (!elf.runpaths.empty())
        for (auto const &path : elf.runpaths)
            os << "? " << path.string() << '\n';

    if (!elf.rpaths.empty())
        for (auto const &path : elf.rpaths)
            os << "? " << path.string() << '\n';

    return os;
}

bool operator==(Elf const &lhs, Elf const &rhs) {
    return lhs.abs_path == rhs.abs_path;
}

struct PathHash {
    size_t operator()(fs::path const &path) const {
        return fs::hash_value(path);
    }
};

// Applies substitutions like $ORIGIN := current work directory
fs::path apply_substitutions(fs::path const &rpath, fs::path const &cwd) {
    if (rpath.begin() == rpath.end() || *rpath.begin() != "$ORIGIN")
        return rpath;

    fs::path concatted_path = cwd;

    for (auto it = ++rpath.begin(); it != rpath.end(); ++it)
        concatted_path /= *it;

    return concatted_path;
}

// Turns path1:path2 into [path1, path2]
std::vector<fs::path> split_paths(std::string const &raw_rpath, fs::path cwd) {
    std::istringstream buffer(raw_rpath);
    std::string path;

    std::vector<fs::path> rpaths;

    while (std::getline(buffer, path, ':'))
        if (!path.empty())
            rpaths.push_back(apply_substitutions(path, cwd));

    return rpaths;
}

std::optional<Elf> from_path(deploy_t type, std::string_view path_str) {
    auto binary_path = fs::canonical(path_str);
    auto cwd = fs::path(binary_path).remove_filename();

    // Read the NEEDED, RPATH and RUNPATH sections
	std::stringstream cmd;
    cmd << "readelf -d " << binary_path.u8string();
    auto result = exec(cmd.str().c_str());

    // NEEDED
    std::vector<fs::path> needed;
    {
        std::regex regex(R"(NEEDED.*?\[(.*?)\])");
        auto begin = std::sregex_iterator(result.begin(), result.end(), regex);
        auto end = std::sregex_iterator();

        for (auto match = begin; match != end; ++match)
            needed.push_back(fs::path{(*match)[1]});
    }

    // RPATH
    std::vector<fs::path> rpaths;
    {
        std::regex regex(R"(RPATH.*?\[(.*?)\])");
        auto begin = std::sregex_iterator(result.begin(), result.end(), regex);
        auto end = std::sregex_iterator();

        for (auto match = begin; match != end; ++match) {
            auto submatch = (*match)[1];
            for (auto const &path : split_paths(submatch, cwd))
                rpaths.push_back(path);
        }
    }

    // RUNPATH
    std::vector<fs::path> runpaths;
    {
        std::regex regex(R"(RUNPATH.*?\[(.*?)\])");
        auto begin = std::sregex_iterator(result.begin(), result.end(), regex);
        auto end = std::sregex_iterator();

        for (auto match = begin; match != end; ++match) {
            auto submatch = (*match)[1];
            for (auto const &path : split_paths(submatch, cwd))
                runpaths.push_back(path);
        }
    }


    return Elf{type, binary_path, runpaths, rpaths, needed};
}

std::string_view trim_ld_line(std::string_view line) {
    // Left trim
    auto pos = line.find_first_not_of(" \t");

    if (pos != std::string::npos)
        line.remove_prefix(pos);

    // Remove comments
    pos = line.find_first_of('#');

    if (pos != std::string::npos)
        line.remove_suffix(line.size() - pos);

    // Right trim
    pos = line.find_last_not_of(" \t");

    if (pos != std::string::npos)
        line.remove_suffix(line.size() - pos - 1);
    
    return line;
}

void parse_ld_conf(fs::path conf, std::vector<fs::path> &directories) {

    // Regex that matches include
    std::regex include(R"(^include\s+(.+?)$)");
    std::string line;
    std::ifstream conf_file{conf};
    std::smatch include_match;

    while (std::getline(conf_file, line)) {
        // There's no string_view support for regex...
        line = trim_ld_line(line);

        std::regex_match(line, include_match, include);

        // Is this an include?
        if (include_match.size() == 2) {
            fs::path match = include_match[1].str();
            std::vector<fs::path> includes = cppglob::glob(match, true);

            for (auto const &file : includes)
                parse_ld_conf(file, directories);
        } else if (line.size() > 0) {
            directories.push_back(line);
        }
    }
}

std::vector<fs::path> parse_ld_conf() {
    std::vector<fs::path> directories;
    parse_ld_conf("/etc/ld.so.conf", directories);
    return directories;
}

class DepsTree {

public:
    DepsTree(std::vector<Elf> &&input, std::vector<fs::path> &&search_paths, std::vector<fs::path> && ld_library_paths) : m_top_level(std::move(input)), m_search_paths(std::move(search_paths)), m_ld_library_paths(std::move(ld_library_paths))
    {
        std::vector<fs::path> rpaths;
        for (auto const &elf : m_top_level)
            explore(elf, rpaths);
    }

private:
    void explore(Elf const &elf, std::vector<fs::path> &rpaths, size_t depth = 0) {
        auto indent = std::string(depth, ' ');

        // Early return if already visited?
        if (m_visited.count(elf.abs_path) > 0)
            return;

        m_visited.insert(elf.abs_path);

        // Append the rpaths of the current ELF file.
        std::copy(elf.rpaths.begin(), elf.rpaths.end(), std::back_inserter(rpaths));
        // Prepare all search paths in the correct order
        std::vector<fs::path> all_paths;

        // If there is no RUNPATH and >= 1 RPATH, use RPATH
        if (elf.runpaths.size() == 0 && rpaths.size() > 0)
            std::copy(rpaths.begin(), rpaths.end(), std::back_inserter(all_paths));

        // Use LD_LIBRARY_PATH
        std::copy(m_ld_library_paths.begin(), m_ld_library_paths.end(), std::back_inserter(all_paths));

        // Use RUNPATH
        std::copy(elf.runpaths.begin(), elf.runpaths.end(), std::back_inserter(all_paths));

        // Use generic search paths
        std::copy(m_search_paths.begin(), m_search_paths.end(), std::back_inserter(all_paths));

        for (auto const &so : elf.needed) {
            std::cout << indent << so.string() << '\n';

            // If empty, skip
            if (so.begin() == so.end())
                continue;

            // If there is a slash, interpret this as a relative path
            if (++so.begin() != so.end()) {
                std::cout << indent << "TODO: Follow relative path" << so << '\n';
                continue;
            }

            auto result = try_paths(all_paths, so);

            if (result)
                explore(*result, rpaths, depth + 1);
            else
                std::cout << indent << "ERROR could not find " << so << '\n';
        }
    }

    std::optional<Elf> try_paths(std::vector<fs::path> const &paths, fs::path const &so) {
        for (auto const &path : paths) {
            auto result = search(path, so);
            if (result)
                return result;
        }

        return std::nullopt;
    }

    std::optional<Elf> search(fs::path const &search, fs::path const &so) {
        auto full = search / so;

        if (!fs::exists(full))
            return std::nullopt;

        return from_path(deploy_t::LIBRARY, full.string());
    }

    size_t m_depth = 0;
    std::vector<Elf> m_top_level;
    std::vector<fs::path> m_search_paths;
    std::vector<fs::path> m_ld_library_paths;
    std::unordered_set<fs::path, PathHash> m_visited;
};

int main(int argc, char ** argv) {
    auto search_directories = parse_ld_conf();

    search_directories.push_back("/lib");
    search_directories.push_back("/usr/lib");

    cxxopts::Options options("bundler", "Bundle binaries to a small bundle for linux");

    options.add_options()
      ("d,destination", "Destination", cxxopts::value<std::string>())
      ("e,executable", "Executable", cxxopts::value<std::vector<std::string>>())
      ("l,library", "Shared library", cxxopts::value<std::vector<std::string>>())
      ("ldconf", "Path to ld.conf", cxxopts::value<std::string>()->default_value("/etc/ld.so.conf"))
      ("p,platform", "Platform (e.g. x86_64). If not provided this will be inferred from the auxiliary vector", cxxopts::value<std::string>());

    auto result = options.parse(argc, argv);

    std::string const platform = [&](){
        if (result.count("platform") != 0)
            return result["platform"].as<std::string>();

        return std::string(reinterpret_cast<char const *>(getauxval(AT_PLATFORM)));
    }();

    if (result.count("destination") == 0)
        return -1;

    // Create the basic structure of the deploy dir
    fs::path usr_dir = fs::path(result["destination"].as<std::string>()) / "usr";
    fs::path bin_dir = usr_dir / "bin";
    fs::path lib_dir = usr_dir / "lib";

    fs::create_directories(bin_dir);
    fs::create_directories(lib_dir);

    std::vector<Elf> pool;

    if (result["library"].count()) {
        for (auto const &f : result["library"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::LIBRARY, f);
            if (val != std::nullopt)
                pool.push_back(*val);
        }
    }

    if (result["executable"].count()) {
        for (auto const &f : result["executable"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::EXECUTABLE, f);
            if (val != std::nullopt)
                pool.push_back(*val);
        }
    }


    std::vector<fs::path> ld_library_paths;

    DepsTree tree{std::move(pool), std::move(search_directories), std::move(ld_library_paths)};
}
