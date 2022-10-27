#include "elf_parser.h"
#include "symbol.h"
#include <stdio.h>

/*
 * build:
 * `gcc test.c elf_parser.c -o test`
 *
 * the result shoud be as same as follow command:
 * `readelf -a FILE | grep FUNC | grep -v UND | grep -v IFUNC`
 */

static void find_symbol(uint64_t pc)
{
    symbol_info *sym = get_symbol_by_pc(pc);
    if (sym) {
        printf("0x%lx: is in function %s\n", pc, sym->name);
    } else {
        printf("0x%lx: fail to find symbol\n", pc);
    }

}

int main()
{
    // char str[] = "/lib/x86_64-linux-gnu/libc.so.6";
    // char str[] = "/home/lmj/code/xxx/shared_object/libfoo.so";
    // char str[] = "/lib/loongarch64-linux-gnu/libc.so.6";
    // char str[] = "/home/loongson/code/helloLSs";
    // char str[] = "/home/loongson/code/helloLSd";
    char str[] = "/home/loongson/code/call";
    void *img;
    parse_elf_symbol(str, 0, &img);
    print_collected_symbols();
    
    /* pc -> symbol */
    symbol_info * sym = image_get_symbol_by_name((image *)img, "main");
    find_symbol(sym->addr + sym->size - 4);
    find_symbol(sym->addr + sym->size);
}
