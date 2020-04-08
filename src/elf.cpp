#include <bundler/elf.hpp>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <termcolor/termcolor.hpp>

#include <regex>

std::ostream &operator<<(std::ostream &os, Elf const &elf) {
    os << termcolor::on_green << (elf.type == deploy_t::EXECUTABLE ? "Executable" : "Shared library") << termcolor::reset << ' ';
    os << termcolor::green << elf.abs_path.string() << termcolor::reset << '\n';
    for (auto const &lib : elf.needed)
        os << "+ " << lib.string() << '\n';

    if (!elf.runpaths.empty())
        for (auto const &path : elf.runpaths)
            os << "? " << path.string() << '\n';

    if (!elf.rpaths.empty())
        for (auto const &path : elf.rpaths)
            os << "? " << path.string() << '\n';

    return os;
}

bool operator==(Elf const &lhs, Elf const &rhs) {
    return lhs.abs_path == rhs.abs_path;
}

size_t PathHash::operator()(fs::path const &path) const {
    return fs::hash_value(path);
}

// Applies substitutions like $ORIGIN := current work directory
fs::path apply_substitutions(fs::path const &rpath, fs::path const &cwd) {
	std::string path = rpath;

	std::regex origin{"\\$(ORIGIN|\\{ORIGIN\\})"};
	std::regex lib{"\\$(LIB|\\{LIB\\})"};
	std::regex platform{"\\$(PLATFORM|\\{PLATFORM\\})"};
	
	path = std::regex_replace(path, origin, cwd.string());
	path = std::regex_replace(path, lib, "lib64");
	path = std::regex_replace(path, platform, "x86_64");

	return path;
}

// Turns path1:path2 into [path1, path2]
std::vector<fs::path> split_paths(std::string_view raw_path) {
    std::istringstream buffer(std::string{raw_path});
    std::string path;

    std::vector<fs::path> rpaths;

    while (std::getline(buffer, path, ':'))
        if (!path.empty())
            rpaths.push_back(path);

    return rpaths;
}

std::optional<Elf> from_path(deploy_t type, fs::path path_str) {
    // Extract some data from the elf file.
    std::vector<fs::path> needed, rpaths, runpaths;

    // Make sure the path is absolute for substitutions
    auto cwd = fs::absolute(path_str).remove_filename();

    // use the filename, or the soname if it exists to uniquely identify a shared lib
    std::string name =  path_str.filename();

    // Try to load the ELF file
    ELFIO::elfio elf;
    if (!elf.load(path_str.string()) || elf.get_class() != ELFCLASS64)
        return std::nullopt;

    // Loop over the sections
    for (ELFIO::Elf_Half i = 0; i < elf.sections.size(); ++i) {
        ELFIO::section* sec = elf.sections[i];
        if ( SHT_DYNAMIC == sec->get_type() ) {
            ELFIO::dynamic_section_accessor dynamic(elf, sec);

            ELFIO::Elf_Xword dyn_no = dynamic.get_entries_num();

            // Find the dynamic section
            if (dyn_no <= 0)
                continue;

            // Loop over the entries
            for (ELFIO::Elf_Xword i = 0; i < dyn_no; ++i) {
                ELFIO::Elf_Xword tag   = 0;
                ELFIO::Elf_Xword value = 0;
                std::string str;
                dynamic.get_entry(i, tag, value, str);

                if (tag == DT_NEEDED) {
                    needed.push_back(str);
                } else if (tag == DT_RUNPATH) {
                    for (auto const &path : split_paths(str))
                        runpaths.push_back(apply_substitutions(path, cwd));
                } else if (tag == DT_RPATH) {
                    for (auto const &path : split_paths(str))
                        rpaths.push_back(apply_substitutions(path, cwd));
                } else if (tag == DT_SONAME) {
                    name = str;
                } else if (tag == DT_NULL) {
                    break;
                }
            }
        }
    }

    return Elf{type, name, path_str, runpaths, rpaths, needed};
}