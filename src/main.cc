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

#include <cxxopts/cxxopts.hpp>
#include <termcolor/termcolor.hpp>
#include <cppglob/glob.hpp>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

#include "excludelist.h"

namespace fs = std::filesystem;

enum class deploy_t { EXECUTABLE, LIBRARY };

struct Elf {
    deploy_t type;
    std::string name;
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
	std::string path = rpath;

	std::regex origin{"\\$(ORIGIN|\\{ORIGIN\\})"};
	std::regex lib{"\\$(LIB|\\{LIB\\})"};
	std::regex platform{"\\$(PLATFORM|\\{PLATFORM\\})"};
	
	path = std::regex_replace(path, origin, cwd.string());
	path = std::regex_replace(path, lib, "lib64");
	path = std::regex_replace(path, platform, "x86_64");

	return path;
}

// Turns path1:path2 into [path1, path2]
std::vector<fs::path> split_paths(std::string_view raw_path) {
    std::istringstream buffer(std::string{raw_path});
    std::string path;

    std::vector<fs::path> rpaths;

    while (std::getline(buffer, path, ':'))
        if (!path.empty())
            rpaths.push_back(path);

    return rpaths;
}

std::optional<Elf> from_path(deploy_t type, fs::path path_str) {
    // Extract some data from the elf file.
    std::vector<fs::path> needed, rpaths, runpaths;

    // Make sure the path is fully canonicalized
    auto binary_path = fs::canonical(path_str);
    auto cwd = fs::path(binary_path).remove_filename();

    // use the filename, or the soname if it exists to uniquely identify a shared lib
    std::string name =  path_str.filename();

    // Try to load the ELF file
    ELFIO::elfio elf;
    if (!elf.load(binary_path.string()) || elf.get_class() != ELFCLASS64)
        return std::nullopt;

    // Loop over the sections
    for (ELFIO::Elf_Half i = 0; i < elf.sections.size(); ++i) {
        ELFIO::section* sec = elf.sections[i];
        if ( SHT_DYNAMIC == sec->get_type() ) {
            ELFIO::dynamic_section_accessor dynamic(elf, sec);

            ELFIO::Elf_Xword dyn_no = dynamic.get_entries_num();

            // Find the dynamic section
            if (dyn_no <= 0)
                continue;

            // Loop over the entries
            for (ELFIO::Elf_Xword i = 0; i < dyn_no; ++i) {
                ELFIO::Elf_Xword tag   = 0;
                ELFIO::Elf_Xword value = 0;
                std::string str;
                dynamic.get_entry(i, tag, value, str);

                if (tag == DT_NEEDED) {
                    needed.push_back(str);
                } else if (tag == DT_RUNPATH) {
                    for (auto const &path : split_paths(str))
                        runpaths.push_back(apply_substitutions(path, cwd));
                } else if (tag == DT_RPATH) {
                    for (auto const &path : split_paths(str))
                        rpaths.push_back(apply_substitutions(path, cwd));
                } else if (tag == DT_SONAME) {
                    name = str;
                } else if (tag == DT_NULL) {
                    break;
                }
            }
        }
    }

    return Elf{type, name, binary_path, runpaths, rpaths, needed};
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

std::vector<fs::path> parse_ld_conf(std::string_view path) {
    std::vector<fs::path> directories;
    parse_ld_conf(path, directories);
    return directories;
}

 std::string repeat(std::string_view input, size_t num)
 {
    std::ostringstream os;
    std::fill_n(std::ostream_iterator<std::string_view>(os), num, input);
    return os.str();
 }

class DepsTree {

public:
    DepsTree(std::vector<Elf> &&input, std::vector<fs::path> &&search_paths, std::vector<fs::path> && ld_library_paths) : m_top_level(std::move(input)), m_search_paths(std::move(search_paths)), m_ld_library_paths(std::move(ld_library_paths))
    {
        std::vector<fs::path> rpaths;
        for (auto const &elf : m_top_level)
            explore(elf, rpaths);
    }

    std::vector<Elf> const &get_deps() const {
        return m_all_binaries;
    }


private:
    void explore(Elf const &elf, std::vector<fs::path> &rpaths, size_t depth = 0) {
        auto indent = repeat("|  ", depth == 0 ? 0 : depth - 1) + (depth == 0 ? "" : "|--");

        auto cached = m_visited.count(elf.name) > 0;
        auto search = std::find(generatedExcludelist.begin(), generatedExcludelist.end(), elf.name);
        auto excluded = search != generatedExcludelist.end();

        std::cout << indent << (excluded ? termcolor::red : cached ? termcolor::blue : termcolor::green) << elf.name << (excluded ? " (excluded)\n" : cached ? " (visited)\n" : "\n") << termcolor::reset;

        // Early return if already visited?
        if (cached || excluded) return;

        // Cache
        m_visited.insert(elf.name);
        m_all_binaries.push_back(elf);

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
            // If empty, skip
            if (so.begin() == so.end())
                continue;

            auto result = [&](){

                // If it is a path (more than just a so name), follow this.
                if (++so.begin() != so.end()) {
                    auto full_path = so; 

                    if (so.is_relative()) {
                        auto cwd = elf.abs_path;
                        cwd.remove_filename();
                        full_path = cwd / so;
                    }

                    return fs::exists(full_path) ? from_path(deploy_t::LIBRARY, full_path.string())
                                                 : std::nullopt;
                } else {
                    return try_paths(all_paths, so);
                }
            }();

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

    std::vector<Elf> m_all_binaries;
};

// Copy binaries over, change their rpath if they have it, and strip them
void deploy(std::vector<Elf> const &deps, fs::path const &bin, fs::path const &lib, fs::path const &strip, fs::path const &chrpath) {
    for (auto const &elf : deps) {
        fs::path new_path = (elf.type == deploy_t::EXECUTABLE ? bin : lib) / elf.name;
        fs::copy_file(elf.abs_path, new_path, fs::copy_options::overwrite_existing);

        std::cout << termcolor::green << elf.abs_path << termcolor::reset << " => " << termcolor::green << new_path << termcolor::reset << '\n';

        auto rpath = (elf.type == deploy_t::EXECUTABLE ? "\\$ORIGIN/../lib" : "\\$ORIGIN");

		// Silently patch the rpath and strip things; let's not care if it fails.
        std::stringstream chrpath_cmd;
        chrpath_cmd << chrpath << " -c -r \"" << rpath << "\" " << new_path;
        exec(chrpath_cmd.str().c_str());

        std::stringstream strip_cmd;
        strip_cmd << strip << ' ' << new_path;
        exec(strip_cmd.str().c_str());
    }
}

int main(int argc, char ** argv) {
    cxxopts::Options options("bundler", "Bundle binaries to a small bundle for linux");

    // Use the strip and chrpath that we ship if we can detect them
    std::string strip = "strip";
    std::string chrpath = "chrpath";

    options.add_options()
      ("d,destination", "Destination", cxxopts::value<std::string>())
      ("e,executable", "Executable", cxxopts::value<std::vector<std::string>>())
      ("l,library", "Shared library", cxxopts::value<std::vector<std::string>>())
      ("r,exclude", "Exclude library", cxxopts::value<std::vector<std::string>>())
      ("ldconf", "Path to ld.conf", cxxopts::value<std::string>()->default_value("/etc/ld.so.conf"));

    auto result = options.parse(argc, argv);

    std::vector<Elf> pool;

    if (result["library"].count()) {
        for (auto const &f : result["library"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::LIBRARY, f);
            if (val != std::nullopt)
                pool.push_back(*val);
        }
    }

    if (result["executable"].count()) {
        for (auto f : result["executable"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::EXECUTABLE, f);
            if (val != std::nullopt)
                pool.push_back(*val);
        }
    }

    if (result["exclude"].count()) {
        auto const &list = result["exclude"].as<std::vector<std::string>>();
        std::copy(list.begin(), list.end(), std::back_inserter(generatedExcludelist));
    }

    // Fill ld library path from the env variable
    std::vector<fs::path> ld_library_paths;
    auto env = std::getenv("LD_LIBRARY_PATH");
    if (env != nullptr)
        for (auto const &path : split_paths(std::string(env)))
            ld_library_paths.push_back(path);

    // Default search paths is ldconfig + /lib + /usr/lib
    auto search_directories = parse_ld_conf(result["ldconf"].as<std::string>());
    search_directories.push_back("/lib");
    search_directories.push_back("/usr/lib");

    // Walk the dependency tree
    std::cout << termcolor::bold << "Dependency tree" << termcolor::reset << '\n';
    DepsTree tree{std::move(pool), std::move(search_directories), std::move(ld_library_paths)};

    std::cout << '\n';

    // And deploy the binaries if requested.
    if (result.count("destination") == 1) {
        fs::path usr_dir = fs::path(result["destination"].as<std::string>()) / "usr";
        fs::path bin_dir = usr_dir / "bin";
        fs::path lib_dir = usr_dir / "lib";

        std::cout << termcolor::bold << "Deploying to " << usr_dir << termcolor::reset << '\n';

        fs::create_directories(bin_dir);
        fs::create_directories(lib_dir);

        deploy(tree.get_deps(), bin_dir, lib_dir, strip, chrpath);
    }
}
