#include <libtree/ld.hpp>
#include <libtree/glob.hpp>

#include <regex>
#include <fstream>

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

#include <iostream>

void parse_ld_conf(fs::path conf, std::vector<fs::path> &directories) {

    // Regex that matches include
    std::string line;
    std::ifstream conf_file{conf};

    while (std::getline(conf_file, line)) {
        // Remove whitespace indent and comments.
        std::string_view stripped = trim_ld_line(line);

        if (stripped.size() == 0) {
            // Skip empty lines
            continue;
        } else if (stripped.rfind("include", 0) == 0 && std::isblank(stripped[7])) {
            // If it starts with `include<blank>`, then strip that off, and
            // remove blanks again.
            stripped.remove_prefix(8);
            stripped = trim_ld_line(stripped);

            // absolutify the include path.
            auto include_path = conf.parent_path() / fs::path{stripped};
            std::vector<fs::path> includes = glob_wrapper(include_path);

            // Recursively parse the globbed includes
            for (auto const &file : includes)
                parse_ld_conf(file, directories);
        } else {
            // Otherwise assume this guy is a directory.
            directories.push_back(stripped);
        }
    }
}

std::vector<fs::path> parse_ld_conf(std::string_view path) {
    std::vector<fs::path> directories;
    parse_ld_conf(path, directories);
    return directories;
}