#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <glob.h>
#include <sys/stat.h>

// TODO: rpath substitution ${LIB} / $LIB / ${PLATFORM} / $PLATFORM
//       just have to work out how to get the proper LIB/PLATFORM values.

struct h_64 {
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

struct ph_64 {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct dynamic_64 {
    int32_t d_tag;
    uint64_t d_val;
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
#define ERR_INVALID_CLASS 1
#define ERR_INVALID_DATA 1
#define ERR_UNSUPPORTED_ELF_FILE 1
#define ERR_INVALID_HEADER 1

#define MAX_SIZE_T 0xFFFFFFFFFFFFFFFF

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

char *default_paths = "/lib:/lib64:/usr/lib:/usr/lib64";

// large buffer in which to copy rpaths, needed libraries and sonames.
char *buf;
size_t buf_size;

// rpath stack: if lib_a needs lib_b needs lib_c and all have rpaths
// then first lib_c's rpaths are considered, then lib_b's, then lib_a's.
// so this data structure keeps a list of offsets into the string buffer
// where rpaths start, like [lib_a_rpath_offset, lib_b_rpath_offset,
// lib_c_rpath_offset]...
size_t rpath_offsets[16];
char found_all_needed[16];

struct visited_file {
    dev_t st_dev;
    ino_t st_ino;
};

// Keep track of the files we've see
struct visited_file visited_files[128];
size_t visited_files_count;

// Offset for our parsed LD_LIBRARY_PATH
size_t ld_library_path_offset;

void tree_preamble(int depth) {
    if (depth == 0)
        return;

    for (int i = 0; i < depth - 1; ++i) {
        if (found_all_needed[i])
            printf("    ");
        else
            printf("\xe2\x94\x82   "); // "│   "
    }

    if (found_all_needed[depth - 1])
        printf("\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "); // "└── "
    else
        printf("\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 "); // "├── "
}

int recurse(char *current_file, int depth, struct found_t reason);

void check_search_paths(struct found_t reason, char *path, char *rpaths,
                        size_t *needed_not_found, size_t needed_buf_offsets[32],
                        int depth) {
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
            if (recurse(path, depth + 1, reason) == 0) {
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

int interpolate_variables(char *dst, char *src, char *ORIGIN, char *LIB,
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

void copy_from_file(FILE *fptr) {
    size_t offset = buf_size;
    char c;
    while ((c = getc(fptr)) != '\0' && c != EOF)
        buf[buf_size++] = c;
    buf[buf_size++] = '\0';
}

int recurse(char *current_file, int depth, struct found_t reason) {
    FILE *fptr = fopen(current_file, "rb");
    if (fptr == NULL)
        return 1;

    // When we're done recursing, we should give back the memory we've claimed.
    size_t old_buf_size = buf_size;

    // Parse the header
    char e_ident[16];
    if (fread(&e_ident, 16, 1, fptr) != 1)
        return ERR_INVALID_MAGIC;

    if (e_ident[0] != 0x7f || e_ident[1] != 'E' || e_ident[2] != 'L' ||
        e_ident[3] != 'F')
        return ERR_INVALID_MAGIC;

    if (e_ident[4] != '\x01' && e_ident[4] != '\x02')
        return ERR_INVALID_CLASS;

    if (e_ident[5] != '\x01' && e_ident[5] != '\x02')
        return ERR_INVALID_DATA;

    int is_64_bit = e_ident[4] == '\x02';
    int is_little_endian = e_ident[5] == '\x01';

    // For now skip 32 bit and non-little endian
    if (!is_64_bit || !is_little_endian)
        return ERR_UNSUPPORTED_ELF_FILE;

    // And get the type
    struct h_64 header;

    if (fread(&header, sizeof(struct h_64), 1, fptr) != 1)
        return ERR_INVALID_HEADER;

    // Make sure it's an executable or library
    if (header.e_type != ET_EXEC && header.e_type != ET_DYN)
        return 7;

    if (fseek(fptr, header.e_phoff, SEEK_SET) != 0)
        return 8;

    struct ph_64 prog_header;

    uint64_t p_offset = MAX_SIZE_T;

    // map vaddr to file offset
    // TODO: make this dynamic length
    uint64_t offsets[32];
    uint64_t addrs[32];

    uint64_t *offsets_end = &offsets[0];
    uint64_t *addrs_end = &addrs[0];

    for (int i = 0; i < 32; ++i) {
        offsets[i] = MAX_SIZE_T;
        addrs[i] = MAX_SIZE_T;
    }

    for (uint64_t i = 0; i < header.e_phnum; ++i) {
        if (fread(&prog_header, sizeof(struct ph_64), 1, fptr) != 1)
            return 9;

        if (prog_header.p_type == PT_LOAD) {
            *offsets_end++ = prog_header.p_offset;
            *addrs_end++ = prog_header.p_vaddr;
        } else if (prog_header.p_type == PT_DYNAMIC) {
            p_offset = prog_header.p_offset;
        }
    }

    // At this point we're going to store the file as "success"
    struct stat finfo;
    if (fstat(fileno(fptr), &finfo) != 0) {
        fclose(fptr);
        return 54;
    }

    int should_recurse = 1;
    for (size_t i = 0; i < visited_files_count; ++i) {
        if (visited_files[i].st_dev == finfo.st_dev &&
            visited_files[i].st_ino == finfo.st_ino) {
            should_recurse = 0;
            break;
        }
    }

    visited_files[visited_files_count].st_dev = finfo.st_dev;
    visited_files[visited_files_count].st_ino = finfo.st_ino;
    ++visited_files_count;

    // No dynamic section? -- let's just assume that is OK for now.
    // we simply print the filename
    if (p_offset == MAX_SIZE_T) {
        for (int i = 0; i < depth; ++i)
            putchar(' ');
        if (should_recurse == 0)
            printf("\e[1;36m%s\e[0m", current_file);
        else
            printf("\e[1;34m%s\e[0m", current_file);
        switch (reason.how) {
        case RPATH:
            if (reason.depth + 1 == depth)
                printf(" [rpath]");
            else
                printf(" [rpath of %d]", reason.depth);
            break;
        case LD_LIBRARY_PATH:
            printf(" [LD_LIBRARY_PATH]");
            break;
        case RUNPATH:
            printf(" [runpath]");
            break;
        case LD_SO_CONF:
            printf(" [ld.so.conf]");
            break;
        case DIRECT:
            printf(" [direct]");
            break;
        default:
            printf("\n");
            break;
        }
        fclose(fptr);
        return 0;
    }

    // Go to the dynamic section
    if (fseek(fptr, p_offset, SEEK_SET) != 0)
        return 10;

    uint64_t strtab = MAX_SIZE_T;
    uint64_t rpath = MAX_SIZE_T;
    uint64_t runpath = MAX_SIZE_T;
    uint64_t soname = MAX_SIZE_T;

    // Offsets in strtab
    uint64_t neededs[32];
    uint64_t needed_count = 0;

    for (int cont = 1; cont;) {
        struct dynamic_64 dyn;

        if (fread(&dyn, sizeof(struct dynamic_64), 1, fptr) != 1)
            return 11;

        switch (dyn.d_tag) {
        case DT_NULL:
            cont = 0;
            break;
        case DT_STRTAB:
            strtab = dyn.d_val;
            break;
        case DT_RPATH:
            rpath = dyn.d_val;
            break;
        case DT_RUNPATH:
            runpath = dyn.d_val;
            break;
        case DT_NEEDED:
            neededs[needed_count++] = dyn.d_val;
            break;
        case DT_SONAME:
            soname = dyn.d_val;
            break;
        }
    }

    if (strtab == MAX_SIZE_T)
        return 14;

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

    // Current character
    char c;

    // Copy the current soname
    size_t soname_buf_offset = buf_size;
    if (soname != MAX_SIZE_T) {
        if (fseek(fptr, strtab_offset + soname, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 16;
        }
        while ((c = getc(fptr)) != '\0' && c != EOF)
            buf[buf_size++] = c;
        buf[buf_size++] = '\0';
    }

    // No need too recurse deeper? then there's also no reason to find rpaths.
    if (should_recurse == 0) {
        tree_preamble(depth);
        if (soname != MAX_SIZE_T)
            printf("\e[1;34m%s", buf + soname_buf_offset);
        else
            printf("\e[1;34m%s", current_file);
        switch (reason.how) {
        case RPATH:
            if (reason.depth + 1 == depth)
                printf(" [rpath]\e[0m\n");
            else
                printf(" [rpath of %d]\e[0m\n", reason.depth);
            break;
        case LD_LIBRARY_PATH:
            printf(" [LD_LIBRARY_PATH]\e[0m\n");
            break;
        case RUNPATH:
            printf(" [runpath]\e[0m\n");
            break;
        case LD_SO_CONF:
            printf(" [ld.so.conf]\e[0m\n");
            break;
        case DIRECT:
            printf(" [direct]\e[0m\n");
            break;
        case DEFAULT:
            printf(" [default path]\e[0m\n");
            break;
        default:
            printf("\e[0m\n");
            break;
        }
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
    if (rpath == MAX_SIZE_T) {
        rpath_offsets[depth] = MAX_SIZE_T;
    } else {
        rpath_offsets[depth] = buf_size;
        if (fseek(fptr, strtab_offset + rpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 1;
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
    if (runpath != MAX_SIZE_T) {
        if (fseek(fptr, strtab_offset + runpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 1;
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
            return 1;
        }
        copy_from_file(fptr);
    }

    fclose(fptr);

    // We have found something, so print it, maybe by soname.
    tree_preamble(depth);

    if (soname != MAX_SIZE_T)
        printf("\e[1;36m%s\e[0m", buf + soname_buf_offset);
    else
        printf("\e[1;36m%s\e[0m", current_file);
    switch (reason.how) {
    case RPATH:
        if (reason.depth + 1 == depth)
            printf(" \e[0;33m[rpath]\e[0m\n");
        else
            printf(" \e[0;33m[rpath of %d]\e[0m\n", reason.depth);
        break;
    case LD_LIBRARY_PATH:
        printf(" \e[0;33m[LD_LIBRARY_PATH]\e[0m\n");
        break;
    case RUNPATH:
        printf(" \e[0;33m[runpath]\e[0m\n");
        break;
    case DIRECT:
        printf(" \e[0;33m[direct]\e[0m\n");
        break;
    case LD_SO_CONF:
        printf(" \e[0;33m[ld.so.conf]\e[0m\n");
        break;
    case DEFAULT:
        printf(" \e[0;33m[default path]\e[0m\n");
        break;
    default:
        printf("\n");
        break;
    }

    // Buffer for the full search path
    char path[4096];

    size_t needed_not_found = needed_count;

    if (needed_not_found == 0)
        goto success;

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
                printf("\e[1;31m%s is not absolute\e[0m\n", name);
            } else if (recurse(name, depth + 1,
                               (struct found_t){.how = DIRECT, .depth = 0}) !=
                       0) {
                tree_preamble(depth + 1);
                printf("\e[1;31m%s not found\e[0m\n", name);
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
    if (runpath == MAX_SIZE_T) {
        // We have a stack of rpaths, try them all, starting with one set at
        // this lib, then the parents.
        for (int j = depth; j >= 0; --j) {
            if (needed_not_found == 0)
                break;
            if (rpath_offsets[j] == MAX_SIZE_T)
                continue;
            char *rpaths = buf + rpath_offsets[j];
            check_search_paths((struct found_t){.how = RPATH, .depth = j}, path,
                               rpaths, &needed_not_found, needed_buf_offsets,
                               depth);
        }
    }

    if (needed_not_found == 0)
        goto success;

    // Then try LD_LIBRARY_PATH, if we have it.
    if (ld_library_path_offset != MAX_SIZE_T) {
        char *ld_library_paths = buf + ld_library_path_offset;
        check_search_paths((struct found_t){.how = LD_LIBRARY_PATH, .depth = 0},
                           path, ld_library_paths, &needed_not_found,
                           needed_buf_offsets, depth);
    }

    if (needed_not_found == 0)
        goto success;

    // Then consider runpaths
    if (runpath != MAX_SIZE_T) {
        char *runpaths = buf + runpath_buf_offset;
        check_search_paths((struct found_t){.how = RUNPATH, .depth = 0}, path,
                           runpaths, &needed_not_found, needed_buf_offsets,
                           depth);
    }

    if (needed_not_found == 0)
        goto success;

    // Check ld.so.conf paths
    check_search_paths((struct found_t){.how = LD_SO_CONF, .depth = 0}, path,
                       buf, &needed_not_found, needed_buf_offsets, depth);

    // Then consider standard paths
    check_search_paths((struct found_t){.how = DEFAULT, .depth = 0}, path,
                       default_paths, &needed_not_found, needed_buf_offsets,
                       depth);

    if (needed_not_found == 0)
        goto success;

    // Finally summarize those that could not be found.
    for (size_t i = 0; i < needed_not_found; ++i) {
        found_all_needed[depth] = i + 1 >= needed_not_found;
        tree_preamble(depth + 1);
        printf("\e[1;31m%s not found\e[0m\n", buf + needed_buf_offsets[i]);
    }

    // If anything was not found, we print the search paths in order they are
    // considered.
    char *indent = malloc(6 * depth);
    char *p = indent;
    for (int i = 0; i < depth - 1; ++i) {
        if (found_all_needed[i]) {
            memcpy(p, "    ", 4);
            p += 4;
        } else {
            memcpy(p, "\xe2\x94\x82   ", 6);
            p += 6;
        }
    }
    strcpy(p, "    ");

    printf(
        "%s\e[1;31m^^^^\e[0m\n%s \e[1;90mThe following paths were considered\n",
        indent, indent);

    // Consider rpaths only when runpath is empty
    if (runpath != MAX_SIZE_T) {
        printf("%s 1. rpaths [skipped: runpath is used]:\n", indent);
    } else {
        printf("%s 1. rpaths:\n", indent);
    }

    for (int j = depth; j >= 0; --j)
        if (rpath_offsets[j] != MAX_SIZE_T)
            printf("%s - %s\n", indent, buf + rpath_offsets[j]);

    if (ld_library_path_offset == MAX_SIZE_T) {
        printf("%s 2. LD_LIBRARY_PATH was not set\n", indent);
    } else {
        printf("%s 2. LD_LIBRARY_PATH:\n%s    - %s\n", indent, indent,
               buf + ld_library_path_offset);
    }

    if (runpath == MAX_SIZE_T) {
        printf("%s 3. runpath was not set\n", indent);
    } else {
        printf("%s 3. runpath:\n%s    - %s\n", indent, indent,
               buf + runpath_buf_offset);
    }

    printf("%s 4. ld.so.conf:\n%s    - %s\n", indent, indent, buf);
    printf("%s 5. Standard paths:\n%s    - %s\n", indent, indent,
           default_paths);
    printf("\e[0m");
    free(indent);

success:
    buf_size = old_buf_size;
    return 0;
}

int parse_ld_conf(char *path);

int ld_conf_globbing(char *pattern) {
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
        code |= parse_ld_conf(result.gl_pathv[i]);

    globfree(&result);
    return code;
}

int parse_ld_conf(char *path) {
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
            size_t n = strlen(begin);
            memcpy(buf + buf_size, begin, n);
            buf_size += n;
            buf[buf_size++] = ':';
        }
    }

    free(line);
    fclose(fptr);

    return 0;
}

void parse_ld_library_path() {
    char *LD_LIBRARY_PATH = "LD_LIBRARY_PATH";
    ld_library_path_offset = MAX_SIZE_T;
    char *val = getenv(LD_LIBRARY_PATH);

    // not set, so nothing to do.
    if (val == NULL)
        return;

    ld_library_path_offset = buf_size;

    // otherwise, we just copy it over and replace ; with :
    // so that it's similar to rpaths.
    size_t bytes = strlen(val) + 1;

    // include the \0.
    memcpy(buf + buf_size, val, bytes);

    // replace ; with :
    char *search = buf + buf_size;
    while ((search = strchr(search, ';')) != NULL)
        *search++ = ':';

    buf_size += bytes;
}

int print_tree(char *path) {
    // This is where we store rpaths, sonames, needed, search paths.
    // and yes I should fix buffer overflow issues...
    buf = malloc(16 * 1024);
    buf_size = 0;
    visited_files_count = 0;

    // First collect standard paths
    parse_ld_conf("/etc/ld.so.conf");
    parse_ld_library_path();

    // Make sure the last colon is replaced with a null.
    if (buf_size > 0)
        buf[buf_size - 1] = '\0';

    int code = recurse(path, 0, (struct found_t){.how = INPUT, .depth = 0});

    free(buf);

    return code;
}

int main(int argc, char **argv) {
    for (size_t i = 1; i < argc && argv[i][0] == '-'; i++) {
        switch (argv[i][1]) {
        case 'v':
            printf("2.1.0\n");
            return 0;
        default:
            fprintf(stderr, "Usage: %s [-h] [file...]\n", argv[0]);
            return 1;
        }
    }

    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-h] [file...]\n", argv[0]);
        return 1;
    }

    return print_tree(argv[1]);
}
