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

deps::deps(std::vector<Elf> &&input, std::vector<fs::path> &&ld_so_conf, std::vector<fs::path> && ld_library_paths, deps::verbosity_t verbose) 
    : m_top_level(std::move(input)), 
      m_ld_library_paths(std::move(ld_library_paths)),
      m_ld_so_conf(std::move(ld_so_conf)),
      m_verbosity(verbose)
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

std::string deps::get_indent(std::vector<bool> const &done) const {
    std::string indent;

    for (size_t idx = 0; idx + 1 < done.size(); ++idx)
        indent += done[idx] ? "    " : "│   ";
    
    if (done.size() != 0)
        indent += done.back() ? "└── " : "├── ";

    return indent;
}

std::string deps::get_error_indent(std::vector<bool> const &done) const {
    std::string indent;

    for (size_t idx = 0; idx + 1 < done.size(); ++idx)
        indent += done[idx] ? "    " : "│   ";
    
    if (done.size() != 0)
        indent += "    ";

    return indent;
}

void deps::explore(Elf const &elf, std::vector<fs::path> &rpaths, std::vector<bool> &done) {

    auto indent = get_indent(done);
    auto cached = m_visited.count(elf.name) > 0;
    auto search = std::find(generatedExcludelist.begin(), generatedExcludelist.end(), elf.name);
    auto excluded = search != generatedExcludelist.end();

    std::cout << indent << (excluded ? termcolor::magenta : cached ? termcolor::blue : termcolor::cyan);
    if (!excluded && !cached)
        std::cout << termcolor::bold;

    std::cout << elf.name 
              << (excluded ? " (skipped)" : cached ? " (visited)" : "") 
              << termcolor::reset;

    std::cout << (excluded ? termcolor::magenta : cached ? termcolor::blue : termcolor::yellow);
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
    if (m_verbosity != verbosity_t::VERY_VERBOSE && (cached || excluded)) return;

    // Cache
    m_visited.insert(elf.name);
    m_all_binaries.push_back(elf);

    // Append the rpaths of the current ELF file.
    auto total_rpaths = rpaths;
    std::copy(elf.rpaths.begin(), elf.rpaths.end(), std::back_inserter(total_rpaths));

    // Recurse deeper
    done.push_back(false);

    std::vector<std::optional<Elf>> children;

    for (auto const &lib : elf.needed) {
        auto result = locate(elf, lib, total_rpaths, elf.runpaths);

        if (m_verbosity == verbosity_t::NONE && result && std::find(generatedExcludelist.begin(), generatedExcludelist.end(), result->name) != generatedExcludelist.end())
            continue;

        children.push_back(result);
    }

    for (size_t idx = 0; idx < children.size(); ++idx) {

        // Set to true if this is the last child we're visiting.
        if (idx + 1 == children.size())
            done[done.size() - 1] = true;

        auto result = children[idx];

        if (result)
            explore(*result, total_rpaths, done);
        else {
            auto indent = get_indent(done);
            std::string whitespaces = get_error_indent(done);
            std::cout << indent << termcolor::bold << termcolor::red << elf.name << " not found: " << termcolor::reset
                      << termcolor::blue << "RPATH " << termcolor::reset;
            if (total_rpaths.empty())
                std::cout << "(empty)";
            else
                std::copy(total_rpaths.begin(), total_rpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << ' ' << termcolor::blue << "LD_LIBRARY_PATH " << termcolor::reset;
            if (m_ld_library_paths.empty())
                std::cout << "(empty)";
            else
                std::copy(m_ld_library_paths.begin(), m_ld_library_paths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << ' ' << termcolor::blue << "RUNPATH " << termcolor::reset;
            if (elf.runpaths.empty())
                std::cout << "(empty)";
            else
                std::copy(elf.runpaths.begin(), elf.runpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
            std::cout << ' ' << termcolor::blue << "/etc/ld.so.conf " << termcolor::reset;
            if (m_ld_so_conf.empty())
                std::cout << "(empty)";
            else
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