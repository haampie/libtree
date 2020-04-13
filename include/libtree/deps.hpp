#pragma once

#include <libtree/elf.hpp>

#include <vector>
#include <filesystem>
#include <optional>
#include <unordered_set>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

class deps {

public:
    enum class verbosity_t {NONE, VERBOSE, VERY_VERBOSE};

    deps(
        std::vector<Elf> &&input,
        std::vector<fs::path> &&ld_so_conf,
        std::vector<fs::path> &&ld_library_paths,
        std::unordered_set<std::string> &&skip,
        verbosity_t verbose,
        bool print_paths
    );

    std::vector<Elf> const &get_deps() const;

private:
    void explore(Elf const &elf, std::vector<fs::path> &rpaths, std::vector<bool> &done);
    void explore(Elf const &elf, std::vector<fs::path> &rpaths);
    void print_error(fs::path const &lib, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths, std::vector<bool> const &done) const;

    std::string get_indent(std::vector<bool> const &done) const;
    std::string get_error_indent(std::vector<bool> const &done) const;

    std::optional<Elf> locate(Elf const &parent, fs::path const &so, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths);
    std::optional<Elf> locate_directly(Elf const &parent, fs::path const &so);
    std::optional<Elf> locate_by_search(fs::path const &so, std::vector<fs::path> const &rpaths, std::vector<fs::path> const &runpaths);
    std::optional<Elf> find_by_paths(fs::path const &so, std::vector<fs::path> const &paths, found_t tag);

    size_t m_depth = 0;
    std::vector<Elf> m_top_level;
    std::vector<fs::path> m_ld_library_paths;
    std::vector<fs::path> m_ld_so_conf;
    std::vector<fs::path> m_default_paths{"/lib", "/usr/lib"};
    std::unordered_set<fs::path, PathHash> m_visited;
    std::unordered_set<std::string> m_skip;

    std::vector<Elf> m_all_binaries;

    verbosity_t m_verbosity;
    bool m_print_paths;
};
