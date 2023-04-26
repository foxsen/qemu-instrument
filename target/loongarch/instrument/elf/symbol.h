#ifndef _ELF_SYMBOL_H_
#define _ELF_SYMBOL_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct image image;

    typedef struct symbol_info {
        struct image *image;
        const char *name;
        uint64_t addr;
        uint64_t size;
    } symbol_info;

    image *image_alloc(const char *path, uintptr_t load_base);
    void image_add_symbol(image *image, const char * name, uint64_t addr, uint64_t size);

    symbol_info *image_get_symbol_by_name(image *image, const char *name);
    /* use this function get symbol info */
    symbol_info *get_symbol_by_pc(uint64_t pc);
    const char *get_symbol_name_by_pc(uint64_t pc);

    bool is_symbol_name_dupcalited(const char *name);
    void print_collected_symbols(void);

#ifdef __cplusplus
}
#endif

#endif
