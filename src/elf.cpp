#include <libtree/elf.hpp>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <termcolor/termcolor.hpp>

#include <regex>

void remove_relative_and_lexically_normalize(std::vector<fs::path> &paths) {
    // Remove anything that is relative -- we require absolute search paths.
    paths.erase(std::remove_if(paths.begin(), paths.end(), [](fs::path const &x){
        return x.is_relative();
    }), paths.end());

    // Make sure that we don't go beyond the filesystem root. (/../.. is an absolute
    // path, that would be normalized to / through this.) 
    for (auto &p : paths)
        p = p.lexically_normal();
}

static const std::regex s_origin{"\\$(ORIGIN|\\{ORIGIN\\})"};
static const std::regex s_lib{"\\$(LIB|\\{LIB\\})"};
static const std::regex s_platform{"\\$(PLATFORM|\\{PLATFORM\\})"};

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
fs::path apply_substitutions(fs::path const &rpath, fs::path const &cwd, elf_type_t type, std::string const &platform) {
	std::string path = rpath;

    auto substitute_lib = (type == elf_type_t::ELF_32 ? "lib" : "lib64");
	
	path = std::regex_replace(path, s_origin, cwd.string());
	path = std::regex_replace(path, s_lib, substitute_lib);
	path = std::regex_replace(path, s_platform, platform);

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

std::optional<Elf> from_path(deploy_t type, found_t found_via, fs::path const &root, fs::path abs_path, std::string const &platform, std::optional<elf_type_t> required_type) {
    // Extract some data from the elf file.
    std::vector<fs::path> needed, rpaths, runpaths;

    // Make sure the path is absolute for substitutions
    auto cwd = abs_path.parent_path();

    // use the filename, or the soname if it exists to uniquely identify a shared lib
    std::string name = abs_path.filename();

    // Try to load the ELF file
    auto actual_path = root / abs_path.relative_path();
    ELFIO::elfio elf;
    if (!elf.load(actual_path.string()))
        return std::nullopt;

    auto elf_type = elf.get_class() == ELFCLASS64 ? elf_type_t::ELF_64 
                                                  : elf_type_t::ELF_32;

    // Check for mismatch between 64 and 32 bit ELF files.
    if (required_type && *required_type != elf_type)
        return std::nullopt;

    // Loop over the sections
    for (ELFIO::Elf_Half i = 0; i < elf.sections.size(); ++i) {
        ELFIO::section* sec = elf.sections[i];
        if ( SHT_DYNAMIC != sec->get_type() )
            continue;

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
                needed.push_back(fs::path(str).lexically_normal());
            } else if (tag == DT_RUNPATH) {
                for (auto const &path : split_paths(str))
                    runpaths.push_back(apply_substitutions(path, cwd, elf_type, platform));
            } else if (tag == DT_RPATH) {
                for (auto const &path : split_paths(str))
                    rpaths.push_back(apply_substitutions(path, cwd, elf_type, platform));
            } else if (tag == DT_SONAME) {
                name = str;
            } else if (tag == DT_NULL) {
                break;
            }
        }
    }

    remove_relative_and_lexically_normalize(runpaths);
    remove_relative_and_lexically_normalize(rpaths);

    return Elf{type, found_via, elf_type, name, abs_path, runpaths, rpaths, needed};
}
