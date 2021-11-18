#include <libtree/deps.hpp>

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <termcolor/termcolor.hpp>

#include <variant>

bool is_lib(fs::path const &p) {
    auto filename = p.filename().string();

    // Should start with `lib`
    if (filename.find("lib") != 0)
        return false;

    // And the first part of the extension is `.so`
    auto idx = filename.find('.');
    
    if (idx == std::string::npos)
        return false;

    return filename.find("so", idx) == idx + 1;
}

deps::deps(
    fs::path const &root,
    std::vector<Elf> &&input, 
    std::vector<fs::path> &&ld_so_conf, 
    std::vector<fs::path> &&ld_library_paths,
    std::unordered_set<std::string> &&skip,
    std::string const &platform,
    deps::verbosity_t verbose,
    bool print_paths) 
    : m_root(std::move(root)),
      m_top_level(std::move(input)), 
      m_ld_library_paths(std::move(ld_library_paths)),
      m_ld_so_conf(std::move(ld_so_conf)),
      m_skip(std::move(skip)),
      m_platform(std::move(platform)),
      m_verbosity(verbose),
      m_print_paths(print_paths)
{
    // Normalize paths, and remove relative ones.
    remove_relative_and_lexically_normalize(m_ld_library_paths);
    remove_relative_and_lexically_normalize(m_ld_so_conf);

    std::vector<fs::path> rpaths;
    if (m_root != "/")
        std::cout << termcolor::bright_grey << m_root.string() << termcolor::reset << '\n';

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

void deps::explore(Elf const &parent, std::vector<fs::path> &rpaths, std::vector<bool> &done) {
    auto indent = get_indent(done);
    auto cached = m_visited.count(parent.name) > 0;
    auto excluded = m_skip.count(parent.name) > 0;

    std::cout << indent << (excluded ? termcolor::magenta : cached ? termcolor::blue : termcolor::cyan);
    if (!excluded && !cached)
        std::cout << termcolor::bold;

    std::cout << (m_print_paths && fs::exists(m_root / parent.abs_path.relative_path()) ? parent.abs_path.string() : parent.name)
              << (excluded ? " (skipped)" : cached ? " (collapsed)" : "")
              << termcolor::reset;

    std::cout << (excluded ? termcolor::magenta : cached ? termcolor::blue : termcolor::yellow);
    switch(parent.found_via) {
        case found_t::NONE:            break;
        case found_t::DIRECT:          std::cout << " [direct]"; break;
        case found_t::RPATH:           std::cout << " [rpath]"; break;
        case found_t::LD_LIBRARY_PATH: std::cout << " [LD_LIBRARY_PATH]"; break;
        case found_t::RUNPATH:         std::cout << " [runpath]"; break;
        case found_t::LD_SO_CONF:      std::cout << " [ld.so.conf]"; break;
        case found_t::DEFAULT_PATHS:   std::cout << " [default paths]"; break;
    }

    std::cout << termcolor::reset << '\n';

    // Early return if already visited?
    if (m_verbosity != verbosity_t::VERY_VERBOSE && (cached || excluded))
        return;

    // Cache
    m_visited.insert(parent.name);
    m_all_binaries.push_back(parent);

    // Append the rpaths of the current ELF file.
    auto total_rpaths = rpaths;
    std::copy(parent.rpaths.begin(), parent.rpaths.end(), std::back_inserter(total_rpaths));

    // Vec of found and not found
    std::vector<std::variant<Elf, fs::path>> children;

    // First detect all needed libs
    for (auto const &lib : parent.needed) {
        auto result = locate(parent, lib, total_rpaths);

        if (m_verbosity == verbosity_t::NONE && result && m_skip.count(result->name) > 0)
            continue;

        if (result) {
            children.push_back(*result);
        } else {
            children.push_back(lib);
            m_success = false;
        }
    }

    // Recurse deeper
    done.push_back(false);

    for (size_t idx = 0; idx < children.size(); ++idx) {

        // Set to true if this is the last child we're visiting.
        if (idx + 1 == children.size())
            done[done.size() - 1] = true;

        std::visit(overloaded {
            [&](Elf const &lib) { explore(lib, total_rpaths, done); },
            [&](fs::path const &lib) { print_error(lib, total_rpaths, parent.runpaths, done); }
        }, children[idx]);
    }

    done.pop_back();
}

void deps::print_error(fs::path const &lib, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths, std::vector<bool> const &done) const {
    
    auto indent = get_indent(done);
    std::string whitespaces = get_error_indent(done);
    std::cout << indent << termcolor::bold << termcolor::red << lib.string() << " not found: " << termcolor::reset
                << termcolor::blue << "RPATH " << termcolor::reset;
    if (rpaths.empty())
        std::cout << "(empty)";
    else
        std::copy(rpaths.begin(), rpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
    std::cout << ' ' << termcolor::blue << "LD_LIBRARY_PATH " << termcolor::reset;
    if (m_ld_library_paths.empty())
        std::cout << "(empty)";
    else
        std::copy(m_ld_library_paths.begin(), m_ld_library_paths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
    std::cout << ' ' << termcolor::blue << "RUNPATH " << termcolor::reset;
    if (runpaths.empty())
        std::cout << "(empty)";
    else
        std::copy(runpaths.begin(), runpaths.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
    std::cout << ' ' << termcolor::blue << "/etc/ld.so.conf " << termcolor::reset;
    if (m_ld_so_conf.empty())
        std::cout << "(empty)";
    else
        std::copy(m_ld_so_conf.begin(), m_ld_so_conf.end(), std::ostream_iterator<fs::path>(std::cout, ":"));
    std::cout << '\n';
}

std::optional<Elf> deps::locate(Elf const &parent, fs::path const &needed, std::vector<fs::path> const &rpaths) {
    // Notice: `needed` is already lexically normalized.

    // Empty, not sure if that even can happen... but it's a no-go.
    if (needed.empty())
        return std::nullopt;

    // A convoluted way to see if there is just _one_ component
    else if (++needed.begin() == needed.end())
        return locate_by_search(parent, needed, rpaths);

    else
        return locate_directly(parent, needed);
}

std::optional<Elf> deps::locate_by_search(Elf const &parent, fs::path const &needed, std::vector<fs::path> const &rpaths) {
    std::optional<Elf> result{std::nullopt};

    // If there is no RUNPATH and >= 1 RPATH, use RPATH
    if (parent.runpaths.size() == 0 && rpaths.size() > 0) {
        result = find_by_paths(parent, needed, rpaths, found_t::RPATH);
        if (result) return result;
    }

    // Try LD_LIBRARY_PATH
    result = find_by_paths(parent, needed, m_ld_library_paths, found_t::LD_LIBRARY_PATH);
    if (result) return result;

    // Try RUNPATH
    result = find_by_paths(parent, needed, parent.runpaths, found_t::RUNPATH);
    if (result) return result;

    // Try ld.so.conf
    result = find_by_paths(parent, needed, m_ld_so_conf, found_t::LD_SO_CONF);
    if (result) return result;

    // Try standard search paths
    result = find_by_paths(parent, needed, m_default_paths, found_t::DEFAULT_PATHS);
    if (result) return result;

    return result;
}

std::optional<Elf> deps::locate_directly(Elf const &parent, fs::path const &needed) {
    // Note: relative DT_NEEDED is possible, it will be relative to the working
    // directory of the current process, which ... seems very awkward and makes no
    // sense when changing root. We skip those. If you care about this edge case,
    // open an issue please. TODO: inform the user about this?
    if (needed.is_relative())
        return std::nullopt;
    
    // Guard against needed == /../../some_path through normalization.
    auto abs_path = needed.lexically_normal().relative_path();
    auto abs_host_to_lib = m_root / abs_path;

    // TODO: symlinks out of the root dir? 
    if (!fs::exists(abs_host_to_lib))
        return std::nullopt;

    return from_path(
        deploy_t::LIBRARY,
        found_t::DIRECT,
        m_root,
        abs_path,
        m_platform,
        parent.elf_type);
}

std::optional<Elf> deps::find_by_paths(Elf const &parent, fs::path const &needed, std::vector<fs::path> const &paths, found_t tag) {
    for (auto const &path : paths) {
        // Both path and needed are normalized
        auto abs_root_to_lib = path / needed;
        auto abs_host_to_lib = m_root / abs_root_to_lib.relative_path();

        // TODO: symlinks pointing outside root.
        if (!fs::exists(abs_host_to_lib))
            continue;

        auto result = from_path(
            deploy_t::LIBRARY,
            tag,
            m_root,
            abs_root_to_lib,
            m_platform,
            parent.elf_type
        );

        if (result) return result;
    }

    return std::nullopt;
}
