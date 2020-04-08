#pragma once

#include <bundler/elf.hpp>

#include <vector>
#include <filesystem>
#include <optional>
#include <unordered_set>

class deps {

public:
    deps(std::vector<Elf> &&input, std::vector<fs::path> &&search_paths, std::vector<fs::path> && ld_library_paths);

    std::vector<Elf> const &get_deps() const;

private:
    void explore(Elf const &elf, std::vector<fs::path> &rpaths, size_t depth = 0);

    std::optional<Elf> try_paths(std::vector<fs::path> const &paths, fs::path const &so);
    std::optional<Elf> search(fs::path const &search, fs::path const &so);

    size_t m_depth = 0;
    std::vector<Elf> m_top_level;
    std::vector<fs::path> m_search_paths;
    std::vector<fs::path> m_ld_library_paths;
    std::unordered_set<fs::path, PathHash> m_visited;

    std::vector<Elf> m_all_binaries;
};