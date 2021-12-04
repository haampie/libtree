#include <sstream>

#include <libtree/deploy.hpp>
#include <libtree/exec.hpp>

#include <termcolor/termcolor.hpp>

// Copy binaries over, change their rpath if they have it, and strip them
void deploy(std::vector<Elf> const &deps, fs::path const &bin, fs::path const &lib, fs::path const &chrpath_path, fs::path const &strip_path, bool chrpath, bool strip) {
    for (auto const &elf : deps) {
        // Go through all symlinks.
        auto canonical = fs::canonical(elf.abs_path);

        // Copy to the deploy folder
        auto deploy_folder = (elf.type == deploy_t::EXECUTABLE ? bin : lib);
        auto deploy_path = deploy_folder / canonical.filename();
        try {
            fs::copy_file(canonical, deploy_path, fs::copy_options::overwrite_existing);
        } catch (fs::filesystem_error& e) {
            std::cerr << "Could not overwrite existing file. Skipping with error:\n\t"
                      << e.what() << '\n';
        }

        std::cout << termcolor::green << canonical << termcolor::reset << " => " << termcolor::green << deploy_path << termcolor::reset << '\n';

        // Create all symlinks
        for (auto link = elf.abs_path; fs::is_symlink(link) && link.filename() != canonical.filename(); link = fs::read_symlink(link)) {
            auto link_destination = deploy_folder / link.filename();
            fs::remove(link_destination);
            fs::create_symlink(deploy_path.filename(), link_destination);

            std::cout << "  " << termcolor::yellow << "creating symlink " << link_destination << termcolor::reset << '\n';
        }

        auto rpath = (elf.type == deploy_t::EXECUTABLE ? "\\$ORIGIN/../lib" : "\\$ORIGIN");

		// Silently patch the rpath and strip things; let's not care if it fails.
        if (chrpath) {
            std::stringstream chrpath_cmd;
            chrpath_cmd << chrpath_path << " -c -r \"" << rpath << "\" " << deploy_path;
            exec(chrpath_cmd.str().c_str());
        }

        if (strip) {
            std::stringstream strip_cmd;
            strip_cmd << strip_path << ' ' << deploy_path;
            exec(strip_cmd.str().c_str());
        }
    }
}
