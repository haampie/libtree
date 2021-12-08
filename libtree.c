#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <glob.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: rpath substitution ${LIB} / $LIB / ${PLATFORM} / $PLATFORM
//       just have to work out how to get the proper LIB/PLATFORM values.

// Libraries we do not show by default -- this reduces the verbosity quite a
// bit.
char const *exclude_list[] = {
    "libc.so",      "libpthread.so",      "libm.so", "libgcc_s.so",
    "libstdc++.so", "ld-linux-x86-64.so", "libdl.so"};

struct libtree_options {
    int verbosity;
    int path;
};

static inline int host_is_little_endian() {
    int test = 1;
    char *bytes = (char *)&test;
    return bytes[0] == 1;
}

struct header_64 {
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct header_32 {
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct prog_64 {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct prog_32 {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

struct dyn_64 {
    int64_t d_tag;
    uint64_t d_val;
};

struct dyn_32 {
    int32_t d_tag;
    uint32_t d_val;
};

#define ET_EXEC 2
#define ET_DYN 3

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_STRTAB 5
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_RUNPATH 29

#define ERR_INVALID_MAGIC 1
#define ERR_INVALID_CLASS 2
#define ERR_INVALID_DATA 3
#define ERR_INVALID_HEADER 4
#define ERR_INVALID_BITS 5
#define ERR_UNSUPPORTED_ELF_FILE 6
#define ERR_NO_EXEC_OR_DYN 7
#define ERR_INVALID_PHOFF 8
#define ERR_INVALID_PROG_HEADER 9
#define ERR_CANT_STAT 10
#define ERR_INVALID_DYNAMIC_SECTION 11
#define ERR_INVALID_DYNAMIC_ARRAY_ENTRY 12
#define ERR_NO_STRTAB 13
#define ERR_INVALID_SONAME 14
#define ERR_INVALID_RPATH 15
#define ERR_INVALID_RUNPATH 16
#define ERR_INVALID_NEEDED 17
#define ERR_NOT_FOUND 18

#define MAX_OFFSET_T 0xFFFFFFFFFFFFFFFF

#define REGULAR_RED "\033[0;31m"
#define BOLD_RED "\033[1;31m"
#define CLEAR "\033[0m"
#define BOLD_YELLOW "\033[33m"
#define BOLD_CYAN "\033[1;36m"
#define REGULAR_MAGENTA "\033[0;35m"
#define REGULAR_BLUE "\033[0;34m"
#define BRIGHT_BLACK "\033[0;90m"

// don't judge me.
#define LIGHT_HORIZONTAL "\xe2\x94\x80"
#define LIGHT_QUADRUPLE_DASH_VERTICAL "\xe2\x94\x8a"
#define LIGHT_UP_AND_RIGHT "\xe2\x94\x94"
#define LIGHT_VERTICAL "\xe2\x94\x82"
#define LIGHT_VERTICAL_AND_RIGHT "\xe2\x94\x9c"

#define JUST_INDENT "    "
#define LIGHT_VERTICAL_WITH_INDENT LIGHT_VERTICAL "   "

typedef enum { EITHER, BITS32, BITS64 } elf_bits_t;

typedef enum {
    INPUT,
    DIRECT,
    RPATH,
    LD_LIBRARY_PATH,
    RUNPATH,
    LD_SO_CONF,
    DEFAULT
} how_t;

struct found_t {
    how_t how;
    // only set when found by in the rpath NOT of the direct parent.  so, when
    // it is found in a "special" way only rpaths allow, which is worth
    // informing the user about.
    int depth;
};

// large buffer in which to copy rpaths, needed libraries and sonames.
char *buf;
size_t buf_size;
size_t max_buf_size;

// rpath stack: if lib_a needs lib_b needs lib_c and all have rpaths
// then first lib_c's rpaths are considered, then lib_b's, then lib_a's.
// so this data structure keeps a list of offsets into the string buffer
// where rpaths start, like [lib_a_rpath_offset, lib_b_rpath_offset,
// lib_c_rpath_offset]...
size_t rpath_offsets[16];
size_t ld_library_path_offset;
size_t default_paths_offset;
size_t ld_so_conf_offset;

char found_all_needed[16];

struct visited_file {
    dev_t st_dev;
    ino_t st_ino;
};

// Keep track of the files we've see
struct visited_file visited_files[128];
size_t visited_files_count;

int color_output = 0;

static inline void maybe_grow_string_buffer(size_t n) {
    // The likely case of not having to resize
    if (buf_size + n <= max_buf_size)
        return;

    // Otherwise give twice the amount of required space.
    max_buf_size = 2 * (buf_size + n);
    char *res = realloc(buf, max_buf_size);
    if (res == NULL) {
        free(buf);
        exit(1);
    }
    buf = res;
}

static void store_string(char *str) {
    size_t n = strlen(str) + 1;
    maybe_grow_string_buffer(n);
    memcpy(buf + buf_size, str, n);
    buf_size += n;
}

static void copy_from_file(FILE *fptr) {
    char c;
    // TODO: this could be a bit more efficient...
    while ((c = getc(fptr)) != '\0' && c != EOF) {
        maybe_grow_string_buffer(1);
        buf[buf_size++] = c;
    }
    maybe_grow_string_buffer(1);
    buf[buf_size++] = '\0';
}

static int is_in_exclude_list(char *soname) {
    // Get to the end.
    char *start = soname;
    char *end = strrchr(start, '\0');

    // Empty needed string, is that even possible?
    if (start == end)
        return 0;

    --end;

    // Strip "1234567890." from the right.
    while (end != start && ((*end >= '0' && *end <= '9') || *end == '.')) {
        --end;
    }

    // Check if we should skip this one.
    for (size_t j = 0; j < sizeof(exclude_list) / sizeof(char *); ++j) {
        size_t len = strlen(exclude_list[j]);
        if (strncmp(start, exclude_list[j], len) != 0)
            continue;
        return 1;
    }

    return 0;
}

static void tree_preamble(int depth) {
    if (depth == 0)
        return;

    for (int i = 0; i < depth - 1; ++i) {
        if (found_all_needed[i])
            fputs(JUST_INDENT, stdout);
        else
            fputs(LIGHT_VERTICAL_WITH_INDENT, stdout);
    }

    if (found_all_needed[depth - 1])
        fputs(LIGHT_UP_AND_RIGHT LIGHT_HORIZONTAL LIGHT_HORIZONTAL " ", stdout);
    else
        fputs(LIGHT_VERTICAL_AND_RIGHT LIGHT_HORIZONTAL LIGHT_HORIZONTAL " ",
              stdout);
}

static int recurse(char *current_file, int depth, struct libtree_options *opts,
                   elf_bits_t bits, struct found_t reason);

static void check_search_paths(struct found_t reason, char *path, char *rpaths,
                               size_t *needed_not_found,
                               size_t needed_buf_offsets[32], int depth,
                               struct libtree_options *opts, elf_bits_t bits) {
    while (*rpaths != '\0') {
        // First remove trailing colons
        for (; *rpaths == ':' && *rpaths != '\0'; ++rpaths) {
        }

        // Check if it was only colons
        if (*rpaths == '\0')
            return;

        // Copy the search path until the first \0 or :
        char *dest = path;
        while (*rpaths != '\0' && *rpaths != ':')
            *dest++ = *rpaths++;

        // Add a separator if necessary
        if (*(dest - 1) != '/')
            *dest++ = '/';

        // Keep track of the end of the current search path.
        char *search_path_end = dest;

        // Try to open it -- if we've found anything, swap it with the back.
        for (size_t i = 0; i < *needed_not_found;) {
            // Append the soname.
            strcpy(search_path_end, buf + needed_buf_offsets[i]);
            found_all_needed[depth] = *needed_not_found <= 1;
            if (recurse(path, depth + 1, opts, bits, reason) == 0) {
                // Found it, so swap out the current soname to the back,
                // and reduce the number of to be found by one.
                size_t tmp = needed_buf_offsets[i];
                needed_buf_offsets[i] =
                    needed_buf_offsets[*needed_not_found - 1];
                needed_buf_offsets[--(*needed_not_found)] = tmp;
            } else {
                ++i;
            }
        }
    }
}

static int interpolate_variables(char *dst, char *src, char *ORIGIN, char *LIB,
                                 char *PLATFORM) {
    // We do *not* write to dst if there is no variable
    // in th src string -- this is a small optimization,
    // cause it's unlikely we find variables (at least,
    // I think).
    char *not_yet_copied = src;
    char *p_src = src;
    char *p_dst = dst;

    while ((p_src = strchr(p_src, '$')) != NULL) {
        // Number of bytes before the dollar.
        size_t n = p_src - not_yet_copied;

        // Go past the dollar.
        ++p_src;

        // what to copy over.
        char *var_val = NULL;

        // Then check if it's a {ORIGIN} / ORIGIN.
        if (strncmp(p_src, "{ORIGIN}", 8) == 0) {
            var_val = ORIGIN;
            p_src += 8;
        } else if (strncmp(p_src, "ORIGIN", 6) == 0) {
            var_val = ORIGIN;
            p_src += 6;
        } else if (strncmp(p_src, "{LIB}", 5) == 0) {
            var_val = LIB;
            p_src += 5;
        } else if (strncmp(p_src, "LIB", 3) == 0) {
            var_val = LIB;
            p_src += 3;
        } else if (strncmp(p_src, "{PLATFORM}", 10) == 0) {
            var_val = PLATFORM;
            p_src += 10;
        } else if (strncmp(p_src, "PLATFORM", 8) == 0) {
            var_val = PLATFORM;
            p_src += 8;
        } else {
            continue;
        }

        // First copy over the string until the variable.
        memcpy(p_dst, not_yet_copied, n);
        p_dst += n;

        // Then set not_yet_copied *after* the variable.
        not_yet_copied = p_src;

        // Then copy the variable value (without null).
        size_t var_len = strlen(var_val);
        memcpy(p_dst, var_val, var_len);
        p_dst += var_len;
    }

    // Did we copy anything? That implies a variable was interpolated.
    // Copy the remainder, including the \0.
    if (not_yet_copied != src) {
        char *end = strchr(src, '\0');
        size_t remainder = end - not_yet_copied + 1;
        memcpy(p_dst, not_yet_copied, remainder);
        p_dst += remainder;
        return p_dst - dst;
    }

    return 0;
}

static void print_colon_delimited_paths(char *start, char *indent) {
    while (1) {
        // Don't print empty string
        if (*start == '\0')
            break;

        // Find the next delimiter after start
        char *next = strchr(start, ':');

        // Don't print empty strings
        if (start == next) {
            ++start;
            continue;
        }

        // If we have found a :, then replace it with a \0
        // so that we can use printf.
        if (next != NULL)
            *next = '\0';

        fputs(indent, stdout);
        fputs(JUST_INDENT, stdout);
        puts(start);

        // We done yet?
        if (next == NULL)
            break;

        // Otherwise put the : back in place and continue.
        *next = ':';
        start = next + 1;
    }
}

static void print_line(int depth, char *name, char *color, int highlight,
                       struct found_t reason) {
    tree_preamble(depth);
    if (color_output)
        fputs(color, stdout);
    fputs(name, stdout);
    if (color_output && highlight)
        fputs(CLEAR " " BOLD_YELLOW, stdout);
    else
        putchar(' ');
    switch (reason.how) {
    case RPATH:
        if (reason.depth + 1 == depth)
            fputs("[rpath]", stdout);
        else
            printf("[rpath of %d]", reason.depth);
        break;
    case LD_LIBRARY_PATH:
        fputs("[LD_LIBRARY_PATH]", stdout);
        break;
    case RUNPATH:
        fputs("[runpath]", stdout);
        break;
    case LD_SO_CONF:
        fputs("[ld.so.conf]", stdout);
        break;
    case DIRECT:
        fputs("[direct]", stdout);
        break;
    case DEFAULT:
        fputs("[default path]", stdout);
        break;
    default:
        break;
    }
    if (color_output)
        fputs(CLEAR "\n", stdout);
    else
        putchar('\n');
}

static int recurse(char *current_file, int depth, struct libtree_options *opts,
                   elf_bits_t parent_bits, struct found_t reason) {
    FILE *fptr = fopen(current_file, "rb");
    if (fptr == NULL)
        return 1;

    // When we're done recursing, we should give back the memory we've claimed.
    size_t old_buf_size = buf_size;

    // Parse the header
    char e_ident[16];
    if (fread(&e_ident, 16, 1, fptr) != 1)
        return ERR_INVALID_MAGIC;

    // Find magic elfs
    if (e_ident[0] != 0x7f || e_ident[1] != 'E' || e_ident[2] != 'L' ||
        e_ident[3] != 'F')
        return ERR_INVALID_MAGIC;

    // Do at least *some* header validation
    if (e_ident[4] != '\x01' && e_ident[4] != '\x02')
        return ERR_INVALID_CLASS;

    if (e_ident[5] != '\x01' && e_ident[5] != '\x02')
        return ERR_INVALID_DATA;

    elf_bits_t curr_bits = e_ident[4] == '\x02' ? BITS64 : BITS32;
    int is_little_endian = e_ident[5] == '\x01';

    // Make sure that we have matching bits with dependent
    if (parent_bits != EITHER && parent_bits != curr_bits)
        return ERR_INVALID_BITS;

    // Make sure that the elf file has a the host's endianness
    // Byte swapping is on the TODO list
    if (is_little_endian ^ host_is_little_endian())
        return ERR_UNSUPPORTED_ELF_FILE;

    // And get the type
    union {
        struct header_64 h64;
        struct header_32 h32;
    } header;

    // Read the (rest of the) elf header
    if (curr_bits == BITS64) {
        if (fread(&header.h64, sizeof(struct header_64), 1, fptr) != 1)
            return ERR_INVALID_HEADER;
        if (header.h64.e_type != ET_EXEC && header.h64.e_type != ET_DYN)
            return ERR_NO_EXEC_OR_DYN;
        if (fseek(fptr, header.h64.e_phoff, SEEK_SET) != 0)
            return ERR_INVALID_PHOFF;
    } else {
        if (fread(&header.h32, sizeof(struct header_32), 1, fptr) != 1)
            return ERR_INVALID_HEADER;
        if (header.h32.e_type != ET_EXEC && header.h32.e_type != ET_DYN)
            return ERR_NO_EXEC_OR_DYN;
        if (fseek(fptr, header.h32.e_phoff, SEEK_SET) != 0)
            return ERR_INVALID_PHOFF;
    }

    // Make sure it's an executable or library
    union {
        struct prog_64 p64;
        struct prog_32 p32;
    } prog;

    // map vaddr to file offset
    // TODO: make this dynamic length
    uint64_t offsets[32];
    uint64_t addrs[32];

    uint64_t *offsets_end = &offsets[0];
    uint64_t *addrs_end = &addrs[0];

    for (int i = 0; i < 32; ++i) {
        offsets[i] = MAX_OFFSET_T;
        addrs[i] = MAX_OFFSET_T;
    }

    // Read the program header.
    uint64_t p_offset = MAX_OFFSET_T;
    if (curr_bits == BITS64) {
        for (uint64_t i = 0; i < header.h64.e_phnum; ++i) {
            if (fread(&prog.p64, sizeof(struct prog_64), 1, fptr) != 1)
                return ERR_INVALID_PROG_HEADER;

            if (prog.p64.p_type == PT_LOAD) {
                *offsets_end++ = prog.p64.p_offset;
                *addrs_end++ = prog.p64.p_vaddr;
            } else if (prog.p64.p_type == PT_DYNAMIC) {
                p_offset = prog.p64.p_offset;
            }
        }
    } else {
        for (uint32_t i = 0; i < header.h32.e_phnum; ++i) {
            if (fread(&prog.p32, sizeof(struct prog_32), 1, fptr) != 1)
                return ERR_INVALID_PROG_HEADER;

            if (prog.p32.p_type == PT_LOAD) {
                *offsets_end++ = prog.p32.p_offset;
                *addrs_end++ = prog.p32.p_vaddr;
            } else if (prog.p32.p_type == PT_DYNAMIC) {
                p_offset = prog.p32.p_offset;
            }
        }
    }

    // At this point we're going to store the file as "success"
    struct stat finfo;
    if (fstat(fileno(fptr), &finfo) != 0) {
        fclose(fptr);
        return ERR_CANT_STAT;
    }

    int seen_before = 0;
    for (size_t i = 0; i < visited_files_count; ++i) {
        if (visited_files[i].st_dev == finfo.st_dev &&
            visited_files[i].st_ino == finfo.st_ino) {
            seen_before = 1;
            break;
        }
    }

    if (!seen_before) {
        visited_files[visited_files_count].st_dev = finfo.st_dev;
        visited_files[visited_files_count].st_ino = finfo.st_ino;
        ++visited_files_count;
    }

    // No dynamic section? (TODO: handle this properly)
    if (p_offset == MAX_OFFSET_T) {
        print_line(depth, current_file, BOLD_CYAN, 1, reason);
        fclose(fptr);
        return 0;
    }

    // Go to the dynamic section
    if (fseek(fptr, p_offset, SEEK_SET) != 0)
        return ERR_INVALID_DYNAMIC_SECTION;

    uint64_t strtab = MAX_OFFSET_T;
    uint64_t rpath = MAX_OFFSET_T;
    uint64_t runpath = MAX_OFFSET_T;
    uint64_t soname = MAX_OFFSET_T;

    // Offsets in strtab
    uint64_t neededs[32];
    uint64_t needed_count = 0;

    for (int cont = 1; cont;) {
        uint64_t d_tag;
        uint64_t d_val;

        if (curr_bits == BITS64) {
            struct dyn_64 dyn;
            if (fread(&dyn, sizeof(struct dyn_64), 1, fptr) != 1)
                return ERR_INVALID_DYNAMIC_ARRAY_ENTRY;
            d_tag = dyn.d_tag;
            d_val = dyn.d_val;

        } else {
            struct dyn_32 dyn;
            if (fread(&dyn, sizeof(struct dyn_32), 1, fptr) != 1)
                return ERR_INVALID_DYNAMIC_ARRAY_ENTRY;
            d_tag = dyn.d_tag;
            d_val = dyn.d_val;
        }

        // Store strtab / rpath / runpath / needed / soname info.
        switch (d_tag) {
        case DT_NULL:
            cont = 0;
            break;
        case DT_STRTAB:
            strtab = d_val;
            break;
        case DT_RPATH:
            rpath = d_val;
            break;
        case DT_RUNPATH:
            runpath = d_val;
            break;
        case DT_NEEDED:
            neededs[needed_count++] = d_val;
            break;
        case DT_SONAME:
            soname = d_val;
            break;
        }
    }

    if (strtab == MAX_OFFSET_T)
        return ERR_NO_STRTAB;

    // find the file offset corresponding to the strtab address
    uint64_t *offset_i = &offsets[0];
    uint64_t *addr_i = &addrs[0];

    // Assume we have a sentinel value.
    // iterate until the next one is larger.
    while (1) {
        if (strtab >= *addr_i && strtab < *(addr_i + 1))
            break;
        ++offset_i;
        ++addr_i;
    }

    uint64_t strtab_offset = *offset_i - *addr_i + strtab;

    // Copy the current soname
    size_t soname_buf_offset = buf_size;
    if (soname != MAX_OFFSET_T) {
        if (fseek(fptr, strtab_offset + soname, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return ERR_INVALID_SONAME;
        }
        copy_from_file(fptr);
    }

    int in_exclude_list =
        soname != MAX_OFFSET_T && is_in_exclude_list(buf + soname_buf_offset);

    // No need to recurse deeper when we aren't in very verbose mode.
    int should_recurse =
        (!seen_before && !in_exclude_list) ||
        (!seen_before && in_exclude_list && opts->verbosity >= 2) ||
        opts->verbosity == 3;

    // Just print the library and return
    if (!should_recurse) {
        char *print_name = soname != MAX_OFFSET_T && !opts->path
                               ? buf + soname_buf_offset
                               : current_file;
        char *print_color = in_exclude_list ? REGULAR_MAGENTA : REGULAR_BLUE;
        print_line(depth, print_name, print_color, 0, reason);
        fclose(fptr);
        goto success;
    }

    // Store the ORIGIN string.
    char origin[4096];
    char *last_slash = strrchr(current_file, '/');
    if (last_slash != NULL) {
        // we're also copying the last /.
        size_t bytes = last_slash - current_file + 1;
        memcpy(origin, current_file, bytes);
        origin[bytes + 1] = '\0';
    } else {
        // this only happens when the input is relative
        origin[0] = '.';
        origin[1] = '/';
        origin[2] = '\0';
    }

    // pointers into the buffer for rpath, soname and needed

    // Copy DT_PRATH
    if (rpath == MAX_OFFSET_T) {
        rpath_offsets[depth] = SIZE_MAX;
    } else {
        rpath_offsets[depth] = buf_size;
        if (fseek(fptr, strtab_offset + rpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return ERR_INVALID_RPATH;
        }

        copy_from_file(fptr);

        // We store the interpolated string right after the literal copy.
        size_t bytes_written =
            interpolate_variables(buf + buf_size, buf + rpath_offsets[depth],
                                  origin, "LIB", "x86_64");
        if (bytes_written > 0) {
            rpath_offsets[depth] = buf_size;
            buf_size += bytes_written;
        }
    }

    // Copy DT_RUNPATH
    size_t runpath_buf_offset = buf_size;
    if (runpath != MAX_OFFSET_T) {
        if (fseek(fptr, strtab_offset + runpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return ERR_INVALID_RUNPATH;
        }

        copy_from_file(fptr);

        // We store the interpolated string right after the literal copy.
        size_t bytes_written = interpolate_variables(
            buf + buf_size, buf + runpath_buf_offset, origin, "LIB", "x86_64");
        if (bytes_written > 0) {
            runpath_buf_offset = buf_size;
            buf_size += bytes_written;
        }
    }

    // Copy needed libraries.
    size_t needed_buf_offsets[32];
    for (size_t i = 0; i < needed_count; ++i) {
        needed_buf_offsets[i] = buf_size;
        if (fseek(fptr, strtab_offset + neededs[i], SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return ERR_INVALID_NEEDED;
        }
        copy_from_file(fptr);
    }

    fclose(fptr);

    char *print_name = soname == MAX_OFFSET_T || opts->path
                           ? current_file
                           : (buf + soname_buf_offset);

    char *print_color = in_exclude_list
                            ? REGULAR_MAGENTA
                            : seen_before ? REGULAR_BLUE : BOLD_CYAN;

    int highlight = !seen_before && !in_exclude_list;
    print_line(depth, print_name, print_color, highlight, reason);

    // Buffer for the full search path
    char path[4096];

    size_t needed_not_found = needed_count;

    if (needed_not_found == 0)
        goto success;

    // Skip common libraries if not verbose
    if (opts->verbosity == 0) {
        for (size_t i = 0; i < needed_not_found;) {
            // If in exclude list, swap to the back.
            if (is_in_exclude_list(buf + needed_buf_offsets[i])) {
                size_t tmp = needed_buf_offsets[i];
                needed_buf_offsets[i] =
                    needed_buf_offsets[needed_not_found - 1];
                needed_buf_offsets[--needed_not_found] = tmp;
                continue;
            } else {
                ++i;
            }
        }

        if (needed_not_found == 0)
            goto success;
    }

    // First go over absolute paths in needed libs.
    for (size_t i = 0; i < needed_not_found;) {
        char *name = buf + needed_buf_offsets[i];
        if (strchr(name, '/') != NULL) {
            // If it is not an absolute path, we bail, cause it then starts to
            // depend on the current working directory, which is rather
            // nonsensical. This is allowed by glibc though.
            found_all_needed[depth] = needed_not_found <= 1;
            if (name[0] != '/') {
                tree_preamble(depth + 1);
                if (color_output)
                    fputs(BOLD_RED, stdout);
                fputs(name, stdout);
                fputs(" is not absolute", stdout);
                if (color_output)
                    fputs(CLEAR "\n", stdout);
                else
                    putchar('\n');
            } else if (recurse(name, depth + 1, opts, curr_bits,
                               (struct found_t){.how = DIRECT, .depth = 0}) !=
                       0) {
                tree_preamble(depth + 1);
                if (color_output)
                    fputs(BOLD_RED, stdout);
                fputs(name, stdout);
                fputs(" not found", stdout);
                if (color_output)
                    fputs(CLEAR "\n", stdout);
            }

            // Even if not officially found, we mark it as found, cause we
            // handled the error here
            size_t tmp = needed_buf_offsets[i];
            needed_buf_offsets[i] = needed_buf_offsets[needed_not_found - 1];
            needed_buf_offsets[--needed_not_found] = tmp;
        } else {
            ++i;
        }
    }

    if (needed_not_found == 0)
        goto success;

    // Consider rpaths only when runpath is empty
    if (runpath == MAX_OFFSET_T) {
        // We have a stack of rpaths, try them all, starting with one set at
        // this lib, then the parents.
        for (int j = depth; j >= 0; --j) {
            if (needed_not_found == 0)
                break;
            if (rpath_offsets[j] == SIZE_MAX)
                continue;
            check_search_paths((struct found_t){.how = RPATH, .depth = j}, path,
                               buf + rpath_offsets[j], &needed_not_found,
                               needed_buf_offsets, depth, opts, curr_bits);
        }
    }

    if (needed_not_found == 0)
        goto success;

    // Then try LD_LIBRARY_PATH, if we have it.
    if (ld_library_path_offset != SIZE_MAX) {
        check_search_paths((struct found_t){.how = LD_LIBRARY_PATH, .depth = 0},
                           path, buf + ld_library_path_offset,
                           &needed_not_found, needed_buf_offsets, depth, opts,
                           curr_bits);
    }

    if (needed_not_found == 0)
        goto success;

    // Then consider runpaths
    if (runpath != MAX_OFFSET_T) {
        check_search_paths((struct found_t){.how = RUNPATH, .depth = 0}, path,
                           buf + runpath_buf_offset, &needed_not_found,
                           needed_buf_offsets, depth, opts, curr_bits);
    }

    if (needed_not_found == 0)
        goto success;

    // Check ld.so.conf paths
    check_search_paths((struct found_t){.how = LD_SO_CONF, .depth = 0}, path,
                       buf + ld_so_conf_offset, &needed_not_found,
                       needed_buf_offsets, depth, opts, curr_bits);

    // Then consider standard paths
    check_search_paths((struct found_t){.how = DEFAULT, .depth = 0}, path,
                       buf + default_paths_offset, &needed_not_found,
                       needed_buf_offsets, depth, opts, curr_bits);

    if (needed_not_found == 0)
        goto success;

    // Finally summarize those that could not be found.
    for (size_t i = 0; i < needed_not_found; ++i) {
        found_all_needed[depth] = i + 1 >= needed_not_found;
        tree_preamble(depth + 1);
        if (color_output)
            fputs(BOLD_RED, stdout);
        fputs(buf + needed_buf_offsets[i], stdout);
        fputs(" not found", stdout);
        if (color_output)
            fputs(CLEAR "\n", stdout);
        else
            putchar('\n');
    }

    // If anything was not found, we print the search paths in order they are
    // considered.
    char *colored_box =
        color_output
            ? REGULAR_RED JUST_INDENT LIGHT_QUADRUPLE_DASH_VERTICAL CLEAR
            : JUST_INDENT LIGHT_QUADRUPLE_DASH_VERTICAL;
    char *indent = malloc((sizeof(LIGHT_VERTICAL_WITH_INDENT) - 1) * depth +
                          strlen(colored_box));
    char *p = indent;
    for (int i = 0; i < depth - 1; ++i) {
        if (found_all_needed[i]) {
            int len = sizeof(JUST_INDENT) - 1;
            memcpy(p, JUST_INDENT, len);
            p += len;
        } else {
            int len = sizeof(LIGHT_VERTICAL_WITH_INDENT) - 1;
            memcpy(p, LIGHT_VERTICAL_WITH_INDENT, len);
            p += len;
        }
    }
    // dotted | in red
    strcpy(p, colored_box);

    fputs(indent, stdout);
    if (color_output)
        fputs(BRIGHT_BLACK, stdout);
    fputs(" Paths considered in this order:\n", stdout);

    // Consider rpaths only when runpath is empty
    if (runpath != MAX_OFFSET_T) {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 1. rpath is skipped because runpath was set\n", stdout);
    } else {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 1. rpath:\n", stdout);
        for (int j = depth; j >= 0; --j) {
            if (rpath_offsets[j] != SIZE_MAX) {
                fputs(indent, stdout);
                if (color_output)
                    fputs(BRIGHT_BLACK, stdout);
                printf("    depth %d\n", j);
                print_colon_delimited_paths(buf + rpath_offsets[j], indent);
            }
        }
    }

    if (ld_library_path_offset == SIZE_MAX) {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 2. LD_LIBRARY_PATH was not set\n", stdout);
    } else {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 2. LD_LIBRARY_PATH:\n", stdout);
        print_colon_delimited_paths(buf + ld_library_path_offset, indent);
    }

    if (runpath == MAX_OFFSET_T) {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 3. runpath was not set\n", stdout);
    } else {
        fputs(indent, stdout);
        if (color_output)
            fputs(BRIGHT_BLACK, stdout);
        fputs(" 3. runpath:\n", stdout);
        print_colon_delimited_paths(buf + runpath_buf_offset, indent);
    }

    fputs(indent, stdout);
    if (color_output)
        fputs(BRIGHT_BLACK, stdout);
    fputs(" 4. ld.so.conf:\n", stdout);
    print_colon_delimited_paths(buf + ld_so_conf_offset, indent);

    fputs(indent, stdout);
    if (color_output)
        fputs(BRIGHT_BLACK, stdout);
    fputs(" 5. Standard paths:\n", stdout);
    print_colon_delimited_paths(buf + default_paths_offset, indent);

    if (color_output)
        fputs(CLEAR, stdout);

    free(indent);

    buf_size = old_buf_size;
    return ERR_NOT_FOUND;

success:
    // Free memory in our string table
    buf_size = old_buf_size;
    return 0;
}

static int parse_ld_config_file(char *path);

static int ld_conf_globbing(char *pattern) {
    glob_t result;
    memset(&result, 0, sizeof(result));
    int status = glob(pattern, 0, NULL, &result);

    // Handle errors (no result is not an error...)
    switch (status) {
    case GLOB_NOSPACE:
    case GLOB_ABORTED:
        globfree(&result);
        return 1;
    case GLOB_NOMATCH:
        globfree(&result);
        return 0;
    }

    // Otherwise parse the files we've found!
    int code = 0;
    for (size_t i = 0; i < result.gl_pathc; ++i)
        code |= parse_ld_config_file(result.gl_pathv[i]);

    globfree(&result);
    return code;
}

static int parse_ld_config_file(char *path) {
    FILE *fptr = fopen(path, "r");

    if (fptr == NULL)
        return 1;

    size_t len;
    ssize_t nread;
    char *line = NULL;

    while ((nread = getline(&line, &len, fptr)) != -1) {
        char *begin = line;
        // Remove leading whitespace
        for (; isspace(*begin); ++begin) {
        }

        // Remove trailing comments
        char *comment = strchr(begin, '#');
        if (comment != NULL)
            *comment = '\0';

        // Go to the last character in the line
        char *end = strchr(begin, '\0');

        // Remove trailing whitespace
        // although, whitespace is technically allowed in paths :think:
        while (end != begin)
            if (!isspace(*--end))
                break;

        // Skip empty lines
        if (begin == end)
            continue;

        // Put back the end of the string
        end[1] = '\0';

        // 'include ': glob whatever follows.
        if (strncmp(begin, "include", 7) == 0 && isspace(begin[7])) {
            begin += 8;
            // Remove more whitespace.
            for (; isspace(*begin); ++begin) {
            }

            // String can't be empty as it was trimmed and
            // still had whitespace next to include.

            // TODO: check if relative globbing to the
            // current file is supported or not.
            // We do *not* support it right now.
            if (*begin != '/')
                continue;

            ld_conf_globbing(begin);
        } else {
            // Copy over and replace trailing \0 with :.
            store_string(begin);
            buf[buf_size - 1] = ':';
        }
    }

    free(line);
    fclose(fptr);

    return 0;
}

static void parse_ld_so_conf() {
    ld_so_conf_offset = buf_size;
    parse_ld_config_file("/etc/ld.so.conf");

    // Replace the last semicolon with a '\0'
    // if we have a nonzero number of paths.
    if (buf_size > ld_so_conf_offset)
        buf[buf_size - 1] = '\0';
}

static void parse_ld_library_path() {
    char *LD_LIBRARY_PATH = "LD_LIBRARY_PATH";
    ld_library_path_offset = SIZE_MAX;
    char *val = getenv(LD_LIBRARY_PATH);

    // not set, so nothing to do.
    if (val == NULL)
        return;

    ld_library_path_offset = buf_size;

    // otherwise, we just copy it over and replace ; with :
    store_string(val);

    // replace ; with :
    char *search = buf + ld_library_path_offset;
    while ((search = strchr(search, ';')) != NULL)
        *search++ = ':';
}

static void set_default_paths() {
    default_paths_offset = buf_size;
    store_string("/lib:/lib64:/usr/lib:/usr/lib64");
}

static int print_tree(int pathc, char **pathv, struct libtree_options *opts) {
    // This is where we store rpaths, sonames, needed, search paths.
    max_buf_size = 1024;
    buf_size = 0;
    buf = malloc(max_buf_size);

    visited_files_count = 0;

    // First collect standard paths
    parse_ld_so_conf();
    parse_ld_library_path();
    // Make sure the last colon is replaced with a null.

    set_default_paths();

    int last_error = 0;

    for (int i = 0; i < pathc; ++i) {
        int result = recurse(pathv[i], 0, opts, EITHER,
                             (struct found_t){.how = INPUT, .depth = 0});
        if (result != 0)
            last_error = result;
    }

    free(buf);

    return last_error;
}

int main(int argc, char **argv) {
    // Enable or disable colors (no-color.com)
    color_output = getenv("NO_COLOR") == NULL && isatty(STDOUT_FILENO);

    // We want to end up with an array of file names
    // in argv[1] up to argv[positional-1].
    int positional = 1;

    // Default values.
    struct libtree_options opts = {.verbosity = 0, .path = 0};

    int opt_help = 0;
    int opt_version = 0;

    // After `--` we treat everything as filenames, not flags.
    int opt_raw = 0;

    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];

        // Positional args don't start with - or are `-` literal.
        if (opt_raw || *arg != '-' || arg[1] == '\0') {
            argv[positional++] = arg;
            continue;
        }

        // Now we're in flag land!
        ++arg;

        // Long flags
        if (*arg == '-') {
            ++arg;

            // Literal '--'
            if (*arg == '\0') {
                opt_raw = 1;
                continue;
            }

            if (strcmp(arg, "version") == 0) {
                opt_version = 1;
            } else if (strcmp(arg, "path") == 0) {
                opts.path = 1;
            } else if (strcmp(arg, "verbose") == 0) {
                ++opts.verbosity;
            } else if (strcmp(arg, "help") == 0) {
                opt_help = 1;
            } else {
                fprintf(stderr, "Unrecognized flag `--%s`\n", arg);
                return 1;
            }

            continue;
        }

        // Short flags
        for (; *arg != '\0'; ++arg) {
            switch (*arg) {
            case 'h':
                opt_help = 1;
                break;
            case 'p':
                opts.path = 1;
                break;
            case 'v':
                ++opts.verbosity;
                break;
            default:
                fprintf(stderr, "Unrecognized flag `-%c`\n", *arg);
                return 1;
            }
        }
    }

    ++argv;
    --positional;

    // Print a help message on -h, --help or no positional args.
    if (opt_help || (!opt_version && positional == 0)) {
        // clang-format off
        fputs("Show the dynamic dependency tree of ELF files\n"
              "Usage: libtree [OPTION]... [FILE]...\n"
              "\n"
              "  -h, --help     Print help info\n"
              "      --version  Print version info\n"
              "\n"
              "File names starting with '-', for example '-.so', can be specified as follows:\n"
              "  libtree -- -.so\n"
              "\n"
              "A. Locating libs options:\n"
              "  -p, --path     Show the path of libraries instead of the soname\n"
              "  -v             Show libraries skipped by default*\n"
              "  -vv            Show dependencies of libraries skipped by default*\n"
              "  -vvv           Show dependencies of already encountered libraries\n"
              "\n"
              "*) For brevity, the following libraries are not shown by default:\n"
              "   ",
              stdout);
        // clang-format on

        // Print a comma separated list of skipped libraries,
        // with some new lines every now and then to make it readable.
        size_t num_excluded = sizeof(exclude_list) / sizeof(char *);

        size_t cursor_x = 4;
        for (size_t j = 0; j < num_excluded; ++j) {
            cursor_x += strlen(exclude_list[j]);
            if (cursor_x > 60) {
                cursor_x = 4;
                fputs("\n   ", stdout);
            }
            fputs(exclude_list[j], stdout);
            if (j + 1 != num_excluded)
                fputs(", ", stdout);
        }
        fputs(".\n", stdout);

        // Return an error status code if no positional args were passed.
        return !opt_help;
    }

    if (opt_version) {
        puts("3.0.0");
        return 0;
    }

    return print_tree(positional, argv, &opts);
}
