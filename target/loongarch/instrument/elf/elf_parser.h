#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>

void parse_elf_symbol(const char* pathname, uint64_t map_base, void **pp_img);
void parse_elf_symbol_with_fd(int fd, uint64_t map_base, void **pp_img);

#endif
