#pragma once

#include <libtree/elf.hpp>

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// Copy binaries over, change their rpath if they have it, and strip them
void deploy(std::vector<Elf> const &deps, fs::path const &bin, fs::path const &lib, fs::path const &chrpath_path, fs::path const &strip_path, bool chrpath, bool strip);