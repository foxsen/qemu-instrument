/* ref: https://stackoverflow.com/questions/34960383/how-read-elf-header-in-c */
#include "elf_parser.h"
#include <elf.h>

#include <stdio.h>
#include <string.h>

#include "symbol.h"
#include "../util/error.h"

/* typedef struct */
/* { */
/*   unsigned char e_ident[EI_NIDENT];     /1* Magic number and other info *1/ */
/*   Elf64_Half    e_type;                 /1* Object file type *1/ */
/*   Elf64_Half    e_machine;              /1* Architecture *1/ */
/*   Elf64_Word    e_version;              /1* Object file version *1/ */
/*   Elf64_Addr    e_entry;                /1* Entry point virtual address *1/ */
/*   Elf64_Off     e_phoff;                /1* Program header table file offset *1/ */
/*   Elf64_Off     e_shoff;                /1* Section header table file offset *1/ */
/*   Elf64_Word    e_flags;                /1* Processor-specific flags *1/ */
/*   Elf64_Half    e_ehsize;               /1* ELF header size in bytes *1/ */
/*   Elf64_Half    e_phentsize;            /1* Program header table entry size *1/ */
/*   Elf64_Half    e_phnum;                /1* Program header table entry count *1/ */
/*   Elf64_Half    e_shentsize;            /1* Section header table entry size *1/ */
/*   Elf64_Half    e_shnum;                /1* Section header table entry count *1/ */
/*   Elf64_Half    e_shstrndx;             /1* Section header string table index *1/ */
/* } Elf64_Ehdr; */

/* typedef struct */
/* { */
/*   Elf64_Word	sh_name;		/1* Section name (string tbl index) *1/ */
/*   Elf64_Word	sh_type;		/1* Section type *1/ */
/*   Elf64_Xword	sh_flags;		/1* Section flags *1/ */
/*   Elf64_Addr	sh_addr;		/1* Section virtual addr at execution *1/ */
/*   Elf64_Off	sh_offset;		/1* Section file offset *1/ */
/*   Elf64_Xword	sh_size;		/1* Section size in bytes *1/ */
/*   Elf64_Word	sh_link;		/1* Link to another section *1/ */
/*   Elf64_Word	sh_info;		/1* Additional section information *1/ */
/*   Elf64_Xword	sh_addralign;		/1* Section alignment *1/ */
/*   Elf64_Xword	sh_entsize;		/1* Entry size if section holds table *1/ */
/* } Elf64_Shdr; */

/* TODO 一些检查elf格式的函数 */
/* /1* Verify the portions of EHDR within E_IDENT for the target. */
/*    This can be performed before bswapping the entire header.  *1/ */
/* static int elf_check_ident(Elf64_Ehdr *ehdr) */
/* { */
/*     return (ehdr->e_ident[EI_MAG0] == ELFMAG0 */
/*             && ehdr->e_ident[EI_MAG1] == ELFMAG1 */
/*             && ehdr->e_ident[EI_MAG2] == ELFMAG2 */
/*             && ehdr->e_ident[EI_MAG3] == ELFMAG3 */
/*             && ehdr->e_ident[EI_CLASS] == ELF_CLASS */
/*             && ehdr->e_ident[EI_DATA] == ELF_DATA */
/*             && ehdr->e_ident[EI_VERSION] == EV_CURRENT); */
/* } */

/* /1* Verify the portions of EHDR outside of E_IDENT for the target. */
/*    This has to wait until after bswapping the header.  *1/ */
/* static int elf_check_ehdr(Elf64_Ehdr *ehdr) */
/* { */
/*     return (elf_check_arch(ehdr->e_machine) */
/*             && elf_check_abi(ehdr->e_flags) */
/*             && ehdr->e_ehsize == sizeof(struct elfhdr) */
/*             && ehdr->e_phentsize == sizeof(struct elf_phdr) */
/*             && (ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN)); */
/* } */


#include <fcntl.h>
#include <unistd.h>
#include <alloca.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

/* lookup .symtab and .dynsym section */
static bool find_sym_table(Elf64_Shdr *shdr, int shnum, Elf64_Word type, int *sym_idx, int *str_idx)
{
    /* type can be SHT_SYMTAB or SHT_DYNSYM */
    for (int i = 0; i < shnum; ++i) {
        if (shdr[i].sh_type == type) {
            *sym_idx = i;
            *str_idx = shdr[i].sh_link;
            return true;
        }
    }
    return false;
}

void parse_elf_symbol_with_fd(int fd, uint64_t map_base, void **pp_img)
{
    static char filepath[1024];
    static char link[64];

    sprintf(link, "/proc/self/fd/%d", fd);
    int n = readlink(link, filepath, 1024);
    if (n < 0 || n >= 1024) {
        fprintf(stderr, "get file path failed\n");
        return;
    }
    filepath[n] = 0;    /* readlink不会给结尾加NULL */

    lsdebug("read symbol in %s\n", filepath);
    parse_elf_symbol(filepath, map_base, pp_img);
}

/* parse ELF and print all symbols() */
void parse_elf_symbol(const char* pathname, uint64_t map_base, void **pp_img)
{
    Elf64_Ehdr *ehdr = NULL;
    int shnum;
    size_t shsz;
    Elf64_Shdr *shdr = NULL;
    uint64_t secsz;
    int sym_idx;
    int str_idx;
    Elf64_Sym *syms = NULL;
    char *strs = NULL;
    int nsyms;
    *pp_img = NULL;

    int fd = open(pathname, O_RDONLY, 0);
    if (fd < 0) {
        /* printf("Error while loading %s: %s\n", filename, strerror(errno)); */
        perror("open file failed\n");
        return;
    }

    /* 1. read ELF header */
    ehdr = (Elf64_Ehdr *)alloca(sizeof(Elf64_Ehdr));
    if (pread(fd, ehdr, sizeof(Elf64_Ehdr), 0) != sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "read elf header failed\n");
        goto give_up;
    }

    /* 2. check if is an ELF shared object or exec */
    // TODO add more check
    if (!((memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0) && ((ehdr->e_type == ET_DYN) || (ehdr->e_type == ET_EXEC)))) {
        fprintf(stderr, "not a shared object\n");
        goto give_up;
    }

    /* 3. read section headers */
    shnum = ehdr->e_shnum;
    shsz = shnum * ehdr->e_shentsize;
    shdr = (Elf64_Shdr *)alloca(shsz);
    if (pread(fd, shdr, shsz, ehdr->e_shoff) != shsz) {
        fprintf(stderr, "read section headers failed\n");
        goto give_up;
    }

    /* 4. lookup .symtab and .dynsym section */
    /* only need one of both, because dynsym is included in symtab */
    if (!find_sym_table(shdr, shnum, SHT_SYMTAB, &sym_idx, &str_idx)
        && !find_sym_table(shdr, shnum, SHT_DYNSYM, &sym_idx, &str_idx)) {
        fprintf(stderr, "find no symbol table\n");
        goto give_up;
    }

    /* 4.1 read symbol string */
    /* TODO add assert strtab exist */
    secsz = shdr[str_idx].sh_size;
    strs = (char *)malloc(secsz);
    if (!strs || pread(fd, strs, secsz, shdr[str_idx].sh_offset) != secsz) {
        fprintf(stderr, "read .symstr section failed\n");
        goto give_up;
    }

    /* 4.2 read symbol table */
    secsz = shdr[sym_idx].sh_size;
    syms = (Elf64_Sym *)malloc(secsz);
    if (!syms || pread(fd, syms, secsz, shdr[sym_idx].sh_offset) != secsz) {
        fprintf(stderr, "read .symtab section failed\n");
        goto give_up;
    }

    if (secsz / sizeof(Elf64_Sym) > INT_MAX) {
        fprintf(stderr, "Implausibly large symbol table, give up\n");
        goto give_up;
    }

    /* 4.3 scan symbol table, collect functions */
    *pp_img = image_alloc(pathname, map_base);
    nsyms = secsz / sizeof(Elf64_Sym);
    for (int i = 0; i < nsyms; ++i) {
        if (syms[i].st_shndx == SHN_UNDEF
            || syms[i].st_shndx >= SHN_LORESERVE
            || ELF64_ST_TYPE(syms[i].st_info) != STT_FUNC) {
            /* Throw away entries which we do not need.  */
            /* FIXME we ignore IFUNC, which seems merely used by glibc */
            continue;
        }
        /* printf("find symbol: %p: %s\n", (void *)(map_base + syms[i].st_value), strs + syms[i].st_name); */
        image_add_symbol((image *)*pp_img, strs + syms[i].st_name, map_base + syms[i].st_value, syms[i].st_size);
    }
give_up:
    free(strs);
    free(syms);
    close(fd);
}
