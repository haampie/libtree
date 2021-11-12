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


void ensure_root(fs::path const &root, std::vector<fs::path> &paths) {
    // Canonicalize paths
    for (auto &p : paths)
        p = fs::canonical(p);

    // Removes all entries not starting with the given root
    paths.erase(std::remove_if(paths.begin(), paths.end(), [&](fs::path const &p){
        return std::mismatch(root.begin(), root.end(), p.begin(), p.end()).first != root.end();
    }), paths.end());

    // Drop the root.
    std::transform(paths.begin(), paths.end(), paths.begin(), [&](fs::path const &p) {
        fs::path new_p{"/"};
        for (auto part = std::mismatch(root.begin(), root.end(), p.begin(), p.end()).second; part != p.end(); ++part)
            new_p /= *part;
        return new_p;
    });
}

void parse_ld_conf(fs::path const &root, fs::path conf, std::vector<fs::path> &directories) {
    // Note: conf here
    // Regex that matches include
    std::string line;
    std::ifstream conf_file{root / conf.relative_path()};

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

            // `include <path>` allows globbing in `<path>`, where we have to be a bit
            // careful about staying in the root. Also when <path> is relative we do it
            // relative to the conf file's directory.
            auto inc = root / (conf.parent_path() / fs::path(stripped)).relative_path();
            std::vector<fs::path> includes = glob_wrapper(inc);

            // Remove the root suffix after globbing as well as paths outside of the
            // root.
            ensure_root(root, includes);

            for (auto const &file : includes)
                parse_ld_conf(root, file, directories);
        } else {
            auto dir = fs::path(stripped);
            // Otherwise assume this guy is a directory.
            // We skip relative paths here, cause they make little sense.
            if (dir.is_relative()) continue;
            directories.push_back(dir);
        }
    }
}

std::vector<fs::path> parse_ld_conf(fs::path const &root, fs::path path) {
    std::vector<fs::path> directories;
    parse_ld_conf(root, path, directories);
    return directories;
}