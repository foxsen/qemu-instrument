#include <vector>
#include <string>
#include <unordered_map>

#include "symbol.h"

struct image {
    std::string path;
    uintptr_t load_base;
    std::unordered_map<std::string, symbol_info> symbols;

    image(const char *path, uintptr_t load_base) : path(path), load_base(load_base)
    {
    }
};

/* 记录所有加载的image（包括应用程序、interpretor、shared_object） */
static std::vector<image *> images;
/* debug 记录重复的函数符号 */
static std::unordered_map<std::string, std::vector<symbol_info>> debug_dup_symbols;


void *image_alloc(const char *path, uintptr_t load_base)
{
    image *img = new image(path, load_base);
    images.push_back(img);
    return img;
}

void image_collect_symbol(void *p_image, const char *name, uint64_t addr, uint64_t size)
{
    image *img = (image *)p_image;
    std::string symbol(name);
    if (img->symbols.count(symbol) == 1) {
        if (debug_dup_symbols[symbol].size() == 0) {
            debug_dup_symbols[symbol].emplace_back(symbol_info{img->symbols[symbol].addr, img->symbols[symbol].size});
            fprintf(stderr, "duplicate symbols: %s, addr: %lu, size: %lu\n", name, addr, size);
        }
        debug_dup_symbols[symbol].emplace_back(symbol_info{addr, size});
    }
    img->symbols[symbol] = symbol_info{addr, size};
}

symbol_info image_get_symbol_by_name(void *p_image, const char *name)
{
    image *img = (image *)p_image;
    std::string symbol(name);
    if (img->symbols.count(symbol) == 0) {
        return symbol_info{0, 0};
    }
    return img->symbols[symbol];
}


void print_collected_symbols(void)
{
    for (image *img : images) {
        printf("Image: %s\n", img->path.c_str());
        printf("ADDR    SIZE    NAME\n");
        for (auto &p : img->symbols) {
            printf("0x%lx, 0x%lx: %s\n", p.second.addr, p.second.size, p.first.c_str());
        }
        printf("Routines count: %lu\n", img->symbols.size());
    }

    if (debug_dup_symbols.empty()) {
        printf("\nNo duplicated symbols. :)\n");
        return;
    }
    printf("\nDuplicated symbols:\n");
    for (auto &p : debug_dup_symbols) {
        printf("%s:\n", p.first.c_str());
        for (symbol_info &sym : p.second) {
            printf("0x%lxp, 0x%lx\n", sym.addr, sym.size);
        }
        printf("\n");
    }
}

