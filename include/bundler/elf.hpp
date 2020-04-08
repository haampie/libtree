#pragma once

#include <string_view>
#include <iostream>
#include <filesystem>
#include <vector>
#include <optional>

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

std::ostream &operator<<(std::ostream &os, Elf const &elf);

bool operator==(Elf const &lhs, Elf const &rhs);

struct PathHash {
    size_t operator()(fs::path const &path) const;
};

// Applies substitutions like $ORIGIN := current work directory
fs::path apply_substitutions(fs::path const &rpath, fs::path const &cwd);

// Turns path1:path2 into [path1, path2]
std::vector<fs::path> split_paths(std::string_view raw_path);

// Try to create an elf from a path
std::optional<Elf> from_path(deploy_t type, fs::path path_str);