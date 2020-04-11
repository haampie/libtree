#include <libtree/ld.hpp>

#include <regex>
#include <fstream>
#include <cppglob/glob.hpp>

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