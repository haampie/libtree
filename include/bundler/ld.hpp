#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::string_view trim_ld_line(std::string_view line);

void parse_ld_conf(fs::path conf, std::vector<fs::path> &directories);

std::vector<fs::path> parse_ld_conf(std::string_view path);