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

#include "cxxopts.hpp"

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

namespace fs = std::filesystem;

enum class deploy_t { EXECUTABLE, LIBRARY };

struct Elf {
    deploy_t type;
    fs::path abs_path;
    std::vector<fs::path> runpaths;
    std::vector<fs::path> rpaths;
    std::vector<fs::path> needed;
};

bool operator==(Elf const &lhs, Elf const &rhs) {
    return lhs.abs_path == rhs.abs_path;
}

struct ElfHash {
    size_t operator()(Elf const &elf) const {
        return fs::hash_value(elf.abs_path);
    }
};

std::optional<Elf> from_path(deploy_t type, std::string_view str) {
    auto path = fs::absolute(str);

    // Return immediately when the binary does not exist
    if (!fs::exists(path)) {
        return std::nullopt;
    }

    // Read the NEEDED, RPATH and RUNPATH sections
	std::stringstream cmd;
    cmd << "readelf -d " << path.u8string();
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

        for (auto match = begin; match != end; ++match)
            rpaths.push_back(fs::path{(*match)[1]});
    }

    // RUNPATH
    std::vector<fs::path> runpaths;
    {
        std::regex regex(R"(RUNPATH.*?\[(.*?)\])");
        auto begin = std::sregex_iterator(result.begin(), result.end(), regex);
        auto end = std::sregex_iterator();

        for (auto match = begin; match != end; ++match)
            runpaths.push_back(fs::path{(*match)[1]});
    }


    return Elf{type, path, rpaths, runpaths, needed};
}

int main(int argc, char ** argv) {
    cxxopts::Options options("binary-bundler", "Bundle binaries to a small bundle for linux");

    options.add_options()
      ("d,destination", "Destination", cxxopts::value<std::string>())
      ("e,executable", "Executable", cxxopts::value<std::vector<std::string>>())
      ("l,library", "Shared library", cxxopts::value<std::vector<std::string>>());

    auto result = options.parse(argc, argv);

    if (result.count("destination") == 0)
        return -1;

    // Create the basic structure of the deploy dir
    fs::path usr_dir = fs::path(result["destination"].as<std::string>()) / "usr";
    fs::path bin_dir = usr_dir / "bin";
    fs::path lib_dir = usr_dir / "lib";

    fs::create_directories(bin_dir);
    fs::create_directories(lib_dir);

    std::unordered_set<Elf,ElfHash> pool;

    if (result["library"].count()) {
        for (auto const &f : result["library"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::LIBRARY, f);
            if (val != std::nullopt)
                pool.insert(*val);
        }
    }

    if (result["executable"].count()) {
        for (auto const &f : result["executable"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::EXECUTABLE, f);
            if (val != std::nullopt)
                pool.insert(*val);
        }
    }

    if (pool.size() == 0)
        return -1;

    std::cout << "Deploying:\n";
    for (auto const &elf : pool)
        std::cout << elf.abs_path << '\n';
}
