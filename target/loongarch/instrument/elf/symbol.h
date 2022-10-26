#ifndef _ELF_SYMBOL_H_
#define _ELF_SYMBOL_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct symbol_info {
        uint64_t addr;
        uint64_t size;
    } symbol_info;

    void *image_alloc(const char *path, uintptr_t load_base);
    void image_collect_symbol(void *p_image, const char *name, uint64_t addr, uint64_t size);
    symbol_info image_get_symbol_by_name(void *p_image, const char *name);
    void print_collected_symbols(void);

#ifdef __cplusplus
}
#endif

#endif
