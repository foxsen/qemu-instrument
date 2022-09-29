
#include "elf_parser.h"
#include "symbol.h"

/*
 * build:
 * `gcc test.c elf_parser.c -o test`
 *
 * the result shoud be as same as follow command:
 * `readelf -a FILE | grep FUNC | grep -v UND | grep -v IFUNC`
 */
int main()
{
    /* char str[] = "/lib/x86_64-linux-gnu/libc.so.6"; */
    char str[] = "/home/lmj/code/xxx/shared_object/libfoo.so";
    void *img;
    parse_elf_symbol(str, 0, &img);
    print_collected_symbols();
}
