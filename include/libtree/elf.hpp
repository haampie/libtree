#pragma once

#include <string_view>
#include <iostream>
#include <filesystem>
#include <vector>
#include <optional>

namespace fs = std::filesystem;

enum class deploy_t { EXECUTABLE, LIBRARY };

enum class found_t { NONE, DIRECT, RPATH, LD_LIBRARY_PATH, RUNPATH, LD_SO_CONF, DEFAULT_PATHS };

enum class elf_type_t { ELF_32, ELF_64 };

struct Elf {
    deploy_t type;
    found_t found_via;
    elf_type_t elf_type;
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
fs::path apply_substitutions(fs::path const &rpath, fs::path const &cwd, std::string const &platform);

// Turns path1:path2 into [path1, path2]
std::vector<fs::path> split_paths(std::string_view raw_path);

// Try to create an elf from a path
std::optional<Elf> from_path(deploy_t type, found_t found_via, fs::path path_str, std::string const &platform, std::optional<elf_type_t> required_type = std::nullopt);