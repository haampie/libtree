#include "excludelist.h"

#include <cxxopts/cxxopts.hpp>
#include <termcolor/termcolor.hpp>

#include <bundler/elf.hpp>
#include <bundler/ld.hpp>
#include <bundler/deploy.hpp>
#include <bundler/deps.hpp>

namespace fs = std::filesystem;

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

    if (result["executable"].count()) {
        for (auto f : result["executable"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::EXECUTABLE, f);
            if (val != std::nullopt)
                pool.push_back(*val);
        }
    }

    if (result["library"].count()) {
        for (auto const &f : result["library"].as<std::vector<std::string>>()) {
            auto val = from_path(deploy_t::LIBRARY, f);
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
    deps tree{std::move(pool), std::move(search_directories), std::move(ld_library_paths)};

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
