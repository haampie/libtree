#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define ET_EXEC    2
#define ET_DYN     3

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2

#define DT_NULL    0
#define DT_NEEDED  1
#define DT_STRTAB  5
#define DT_SONAME  14
#define DT_RPATH   15
#define DT_RUNPATH 29


#define ERR_INVALID_MAGIC 1
#define ERR_INVALID_CLASS 1
#define ERR_INVALID_DATA 1
#define ERR_UNSUPPORTED_ELF_FILE 1
#define ERR_INVALID_HEADER 1

// large buffer in which to copy rpaths, needed libraries and sonames.
char * buf;
size_t buf_size;


// rpath stack: if lib_a needs lib_b needs lib_c and all have rpaths
// then first lib_c's rpaths are considered, then lib_b's, then lib_a's.
// so this data structure keeps a list of offsets into the string buffer
// where rpaths start, like [lib_a_rpath_offset, lib_b_rpath_offset,
// lib_c_rpath_offset]...
size_t rpath_offsets[16];

int recurse(FILE *fptr, int depth);

void check_rpaths(char *path, char *rpaths, size_t *needed_not_found, size_t needed_buf_offsets[32], int depth) {
    while (*rpaths != '\0') {
        // First remove trailing colons
        for (;*rpaths == ':' && *rpaths != '\0'; ++rpaths);

        // Check if it was only colons
        if (*rpaths == '\0') return;

        // Copy the search path until the first \0 or :
        char *dest = path;
        while (*rpaths != '\0' && *rpaths != ':')
            *dest++ = *rpaths++;

        // Add a separator
        *dest++ = '/';

        // Keep track of the end of the current search path.
        char *search_path_end = dest;

        // Try to open it -- if we've found anything, swap it with the back.
        for (size_t i = 0; i < *needed_not_found;) {
            // Append the soname.
            strcpy(search_path_end, buf + needed_buf_offsets[i]);

            // Try and open
            FILE * test = fopen(path, "rb");

            if (test != NULL) { 
                for (int i = 0; i < depth; ++i) putchar(' ');
                printf("\e[1;36m%s\e[0m\n", buf + needed_buf_offsets[i]);

                // Found it, so swap out the current soname to the back,
                // and reduce the number of to be found by one.
                size_t tmp = needed_buf_offsets[i];
                needed_buf_offsets[i] = needed_buf_offsets[*needed_not_found-1];
                needed_buf_offsets[--(*needed_not_found)] = tmp;

                recurse(test, depth + 1);
            } else {
                ++i;
            }
        }
    }
}

int recurse(FILE *fptr, int depth) {
    // When we're done recursing, we should give back the memory we've claimed.
    size_t old_buf_size = buf_size;

    // parse the header
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

    // for now skip 32 bit and non-little endian
    if (!is_64_bit && !is_little_endian)
        return ERR_UNSUPPORTED_ELF_FILE;

    // and get the type
    struct h_64 header;

    if (fread(&header, sizeof(struct h_64), 1, fptr) != 1)
        return ERR_INVALID_HEADER;

    // Make sure it's an exectuable or library
    if (header.e_type != ET_EXEC && header.e_type != ET_DYN)
        return 7;

    if (fseek(fptr, header.e_phoff, SEEK_SET) != 0)
        return 8;

    struct ph_64 prog_header;

    uint64_t p_offset = 0;

    // map vaddr to file offset
    // TODO: make this dynamic length
    uint64_t offsets[32];
    uint64_t addrs[32];

    uint64_t * offsets_end = &offsets[0];
    uint64_t * addrs_end = &addrs[0];

    for (int i = 0; i < 32; ++i) {
        offsets[i] = 0xffffffffffffffff;
        addrs[i] = 0xffffffffffffffff;
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

    // No dynamic section?
    if (p_offset == 0)
        return 9;

    // Go to the dynamic section
    if (fseek(fptr, p_offset, SEEK_SET) != 0)
        return 10;

    uint64_t strtab = 0xFFFFFFFFFFFFFFFF;
    uint64_t rpath = 0xFFFFFFFFFFFFFFFF;
    uint64_t runpath = 0xFFFFFFFFFFFFFFFF;
    uint64_t soname = 0xFFFFFFFFFFFFFFFF;

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
        case DT_SONAME:
            soname = dyn.d_val;
            break;
        }
    }

    if (strtab == 0xFFFFFFFFFFFFFFFF)
        return 14;

    // find the file offset corresponding to the strtab address
    uint64_t *offset_i = &offsets[0];
    uint64_t *addr_i = &addrs[0];

    // Assume we have a sentinel value.
    // iterate until the next one is larger.
    while (1) {
        if (strtab >= *addr_i && strtab < *(addr_i+1)) break;
        ++offset_i; ++addr_i;
    }

    uint64_t strtab_offset = *offset_i - *addr_i + strtab;

    // Current character
    char c;

    // pointers into the buffer for rpath, soname and needed
    rpath_offsets[depth] = buf_size;

    // Copy DT_PRATH
    if (rpath != 0xFFFFFFFFFFFFFFFF) {
        if (fseek(fptr, strtab_offset + rpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 16;
        }
        while ((c = getc(fptr)) != '\0' && c != EOF)
            buf[buf_size++] = c;
        buf[buf_size++] = '\0';
    }

    // Copy DT_RUNPATH
    size_t runpath_buf_offset = buf_size;
    if (runpath != 0xFFFFFFFFFFFFFFFF) {
        if (fseek(fptr, strtab_offset + runpath, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 16;
        }
        while ((c = getc(fptr)) != '\0' && c != EOF)
            buf[buf_size++] = c;
        buf[buf_size++] = '\0';
    }

    // Copy the current soname
    size_t soname_buf_offset = buf_size;
    if (soname != 0xFFFFFFFFFFFFFFFF) {
        if (fseek(fptr, strtab_offset + soname, SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 16;
        }
        while ((c = getc(fptr)) != '\0' && c != EOF)
            buf[buf_size++] = c;
        buf[buf_size++] = '\0';
    }

    // Copy needed libraries.
    size_t needed_buf_offsets[32];
    for (size_t i = 0; i < needed_count; ++i) {
        needed_buf_offsets[i] = buf_size;
        if (fseek(fptr, strtab_offset + neededs[i], SEEK_SET) != 0) {
            buf_size = old_buf_size;
            return 16;
        }
        while ((c = getc(fptr)) != '\0' && c != EOF)
            buf[buf_size++] = c;
        buf[buf_size++] = '\0';
    }

    fclose(fptr);

    // todo: library path, default path, /etc/ld.so.conf, substitution, stop when already found.

    // Print them too
    // if (rpath != 0xFFFFFFFFFFFFFFFF) {
    //     for (int i = 0; i < depth; ++i) putchar(' ');
    //     printf("rpath = %s\n", buf + rpath_buf_offset);
    // }

    // if (soname != 0xFFFFFFFFFFFFFFFF) {
    //     for (int i = 0; i < depth; ++i) putchar(' ');
    //     printf("soname = %s\n", buf + soname_buf_offset);
    // }

    // for (size_t i = 0; i < needed_count; ++i) {
    //     for (int i = 0; i < depth; ++i) putchar(' ');
    //     printf("needed = %s\n", buf + needed_buf_offsets[i]);
    // }

    // Buffer for the full search path
    char path[4096];

    size_t needed_not_found = needed_count;

    if (needed_not_found == 0) goto success;

    // First go over absolute paths in needed libs.
    for (size_t i = 0; i < needed_not_found;) {
        if (strchr(buf + needed_buf_offsets[i], '/') != NULL) {
            size_t tmp = needed_buf_offsets[i];
            needed_buf_offsets[i] = needed_buf_offsets[needed_not_found-1];
            needed_buf_offsets[--needed_not_found] = tmp;
        } else {
            ++i;
        }
    }

    if (needed_not_found == 0) goto success;

    // Consider rpaths only when runpath is empty
    if (runpath == 0xFFFFFFFFFFFFFFFF) {
        // We have a stack of rpaths, try them all, starting with one set at this lib, then the parents.
        for (int j = depth; j >= 0; --j) {
            if (needed_not_found == 0) break;
            char *rpaths = buf + rpath_offsets[j];
            check_rpaths(path, rpaths, &needed_not_found, needed_buf_offsets, depth);
        }
    }

    if (needed_not_found == 0) goto success;

    // Then consider runpaths
    if (runpath != 0xFFFFFFFFFFFFFFFF) {
        char *runpaths = buf + runpath_buf_offset;
        check_rpaths(path, runpaths, &needed_not_found, needed_buf_offsets, depth);
    }

success:
    buf_size = old_buf_size;
    return 0;
}

int print_tree(char * path) {
    // This is where we store rpaths, sonames, needed.
    buf = malloc(8192);
    buf_size = 0;

    FILE * fptr = fopen(path, "rb");
    if (fptr == NULL) return 1;

    int code = recurse(fptr, 0);

    free(buf);

    return code;
}

int main(int argc, char **argv) {
    for (size_t optind = 1; optind < argc && argv[optind][0] == '-'; optind++) {
        switch (argv[optind][1]) {
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
