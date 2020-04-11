#include <libtree/deps.hpp>

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

deps::deps(std::vector<Elf> &&input, std::vector<fs::path> &&ld_so_conf, std::vector<fs::path> && ld_library_paths, bool verbose) 
    : m_top_level(std::move(input)), 
      m_ld_so_conf(std::move(ld_so_conf)), 
      m_ld_library_paths(std::move(ld_library_paths)),
      m_verbose(verbose)
{
    std::vector<fs::path> rpaths;
    for (auto const &elf : m_top_level)
        explore(elf, rpaths);
}

std::vector<Elf> const &deps::get_deps() const {
    return m_all_binaries;
}

void deps::explore(Elf const &elf, std::vector<fs::path> &rpaths) {
    std::vector<bool> done{};
    explore(elf, rpaths, done);
}

void deps::explore(Elf const &elf, std::vector<fs::path> &rpaths, std::vector<bool> &done) {
    std::string indent;
    for (size_t idx = 0; idx < done.size(); ++idx) {
        if (done[idx]) {
            if (idx + 1 == done.size())
                indent += "└── ";
            else
                indent += "    ";
        } else {
            if (idx + 1 == done.size())
                indent += "├── ";
            else
                indent += "│   ";
        }
    }

    auto cached = m_visited.count(elf.name) > 0;
    auto search = std::find(generatedExcludelist.begin(), generatedExcludelist.end(), elf.name);
    auto excluded = search != generatedExcludelist.end();


    std::cout << indent << (excluded ? termcolor::magenta : cached ? termcolor::blue : termcolor::yellow);
    if (!excluded && !cached)
        std::cout << termcolor::bold;

    std::cout << elf.name 
              << (excluded ? " (skipped)" : cached ? " (visited)" : "") 
              << termcolor::reset;

    std::cout << termcolor::magenta;
    switch(elf.found_via) {
        case found_t::NONE: break;
        case found_t::DIRECT:          std::cout << " [direct]"; break;
        case found_t::RPATH:           std::cout << " [rpath]"; break;
        case found_t::LD_LIBRARY_PATH: std::cout << " [LD_LIBRARY_PATH]"; break;
        case found_t::RUNPATH:         std::cout << " [runpath]"; break;
        case found_t::LD_SO_CONF:      std::cout << " [ld.so.conf]"; break;
        case found_t::DEFAULT_PATHS:   std::cout << " [default paths]"; break;
    }

    std::cout << termcolor::reset << '\n';

    // Early return if already visited?
    if (cached || excluded) return;

    // Cache
    m_visited.insert(elf.name);
    m_all_binaries.push_back(elf);

    // Append the rpaths of the current ELF file.
    auto total_rpaths = rpaths;
    std::copy(elf.rpaths.begin(), elf.rpaths.end(), std::back_inserter(total_rpaths));

    // Recurse deeper
    done.push_back(false);

    for (size_t idx = 0; idx < elf.needed.size(); ++idx) {

        // Set to true if this is the last child we're visiting.
        if (idx + 1 == elf.needed.size())
            done[done.size() - 1] = true;

        auto result = locate(elf, elf.needed[idx], total_rpaths, elf.runpaths);

        if (result)
            explore(*result, total_rpaths, done);
        else {
            std::string whitespaces(indent.size(), ' ');
            std::cout << indent << termcolor::red << termcolor::bold << "ERROR could not find " << elf.name << ". \n" << termcolor::reset
                      << whitespaces << termcolor::blue << "RPATH: " << termcolor::reset;
            std::copy(total_rpaths.begin(), total_rpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << '\n'
                      << whitespaces << termcolor::blue << "RUNPATH: " << termcolor::reset;
            std::copy(elf.runpaths.begin(), elf.runpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << '\n'
                      << whitespaces << termcolor::blue << "LD_LIBRARY_PATH: " << termcolor::reset;
            std::copy(m_ld_library_paths.begin(), m_ld_library_paths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << '\n'
                      << whitespaces << termcolor::blue << "/etc/ld.so.conf: " << termcolor::reset;
            std::copy(m_ld_so_conf.begin(), m_ld_so_conf.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << '\n';
        }
    }

    done.pop_back();
}

std::optional<Elf> deps::locate(Elf const &parent, fs::path const &so, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths) {
    // Empty, not sure if that even can happen... but it's a no-go.
    if (so.empty())
        return std::nullopt;

    // A convoluted way to see if there is just _one_ component
    else if (++so.begin() == so.end())
        return locate_by_search(so, rpaths, runpaths);

    else
        return locate_directly(parent, so);
}

std::optional<Elf> deps::locate_by_search(fs::path const &so, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths) {
    std::optional<Elf> result{std::nullopt};

    // If there is no RUNPATH and >= 1 RPATH, use RPATH
    if (runpaths.size() == 0 && rpaths.size() > 0) {
        result = find_by_paths(so, rpaths, found_t::RPATH);
        if (result) return result;
    }

    // Try LD_LIBRARY_PATH
    result = find_by_paths(so, m_ld_library_paths, found_t::LD_LIBRARY_PATH);
    if (result) return result;

    // Try RUNPATH
    result = find_by_paths(so, runpaths, found_t::RUNPATH);
    if (result) return result;

    // Try ld.so.conf
    result = find_by_paths(so, m_ld_so_conf, found_t::LD_SO_CONF);
    if (result) return result;

    // Try standard search paths
    result = find_by_paths(so, m_default_paths, found_t::DEFAULT_PATHS);
    if (result) return result;

    return result;
}

std::optional<Elf> deps::locate_directly(Elf const &parent, fs::path const &so) {
    auto full_path = so; 

    if (so.is_relative()) {
        auto cwd = parent.abs_path;
        cwd.remove_filename();
        full_path = cwd / so;
    }

    return fs::exists(full_path) ? from_path(deploy_t::LIBRARY, found_t::DIRECT, full_path.string())
                                 : std::nullopt;
}

std::optional<Elf> deps::find_by_paths(fs::path const &so, std::vector<fs::path> const &paths, found_t tag) {
    for (auto const &path : paths) {
        auto full = path / so;

        if (!fs::exists(full))
            continue;

        auto result = from_path(deploy_t::LIBRARY, tag, full.string());

        if (result) return result;
    }

    return std::nullopt;
}