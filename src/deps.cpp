#include <bundler/deps.hpp>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <termcolor/termcolor.hpp>

#include "excludelist.h"

std::string repeat(std::string_view input, size_t num)
{
    std::ostringstream os;
    std::fill_n(std::ostream_iterator<std::string_view>(os), num, input);
    return os.str();
}

deps::deps(std::vector<Elf> &&input, std::vector<fs::path> &&search_paths, std::vector<fs::path> && ld_library_paths) 
    : m_top_level(std::move(input)), 
      m_search_paths(std::move(search_paths)), 
      m_ld_library_paths(std::move(ld_library_paths))
{
    std::vector<fs::path> rpaths;
    for (auto const &elf : m_top_level)
        explore(elf, rpaths);
}

std::vector<Elf> const &deps::get_deps() const {
    return m_all_binaries;
}

void deps::explore(Elf const &elf, std::vector<fs::path> &rpaths, size_t depth) {
    auto indent = repeat("|  ", depth == 0 ? 0 : depth - 1) + (depth == 0 ? "" : "|--");

    auto cached = m_visited.count(elf.name) > 0;
    auto search = std::find(generatedExcludelist.begin(), generatedExcludelist.end(), elf.name);
    auto excluded = search != generatedExcludelist.end();

    std::cout << indent << (excluded ? termcolor::red : cached ? termcolor::blue : termcolor::green) << elf.name << (excluded ? " (excluded)\n" : cached ? " (visited)\n" : "\n") << termcolor::reset;

    // Early return if already visited?
    if (cached || excluded) return;

    // Cache
    m_visited.insert(elf.name);
    m_all_binaries.push_back(elf);

    // Append the rpaths of the current ELF file.
    std::copy(elf.rpaths.begin(), elf.rpaths.end(), std::back_inserter(rpaths));

    // Prepare all search paths in the correct order
    std::vector<fs::path> all_paths;

    // If there is no RUNPATH and >= 1 RPATH, use RPATH
    if (elf.runpaths.size() == 0 && rpaths.size() > 0)
        std::copy(rpaths.begin(), rpaths.end(), std::back_inserter(all_paths));

    // Use LD_LIBRARY_PATH
    std::copy(m_ld_library_paths.begin(), m_ld_library_paths.end(), std::back_inserter(all_paths));

    // Use RUNPATH
    std::copy(elf.runpaths.begin(), elf.runpaths.end(), std::back_inserter(all_paths));

    // Use generic search paths
    std::copy(m_search_paths.begin(), m_search_paths.end(), std::back_inserter(all_paths));

    for (auto const &so : elf.needed) {
        // If empty, skip
        if (so.begin() == so.end())
            continue;

        auto result = [&](){

            // If it is a path (more than just a so name), follow this.
            if (++so.begin() != so.end()) {
                auto full_path = so; 

                if (so.is_relative()) {
                    auto cwd = elf.abs_path;
                    cwd.remove_filename();
                    full_path = cwd / so;
                }

                return fs::exists(full_path) ? from_path(deploy_t::LIBRARY, full_path.string())
                                                : std::nullopt;
            } else {
                return try_paths(all_paths, so);
            }
        }();

        if (result)
            explore(*result, rpaths, depth + 1);
        else {
            std::cout << indent << "ERROR could not find " << so << '\n';
            std::cout << "Search paths: ";
            std::copy(all_paths.begin(), all_paths.end(), std::ostream_iterator<fs::path>(std::cout, "\n"));
            std::cout << "RUNPATHS: ";
            std::copy(elf.runpaths.begin(), elf.runpaths.end(), std::ostream_iterator<fs::path>(std::cout, "\n"));
        }
    }
}

std::optional<Elf> deps::try_paths(std::vector<fs::path> const &paths, fs::path const &so) {
    for (auto const &path : paths) {
        auto result = search(path, so);
        if (result)
            return result;
    }

    return std::nullopt;
}

std::optional<Elf> deps::search(fs::path const &search, fs::path const &so) {
    auto full = search / so;

    if (!fs::exists(full))
        return std::nullopt;

    return from_path(deploy_t::LIBRARY, full.string());
}
