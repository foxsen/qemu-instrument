#include <vector>
#include <string>
#include <unordered_map>

#include "symbol.h"

struct image {
    std::string path;
    uintptr_t load_base;
    std::unordered_map<std::string, uintptr_t> symbols;

    image(const char *path, uintptr_t load_base) : path(path), load_base(load_base)
    {
    }
};

/* 记录所有加载的image（包括应用程序、interpretor、shared_object） */
static std::vector<image *> images;
static std::unordered_map<std::string, std::vector<uintptr_t>> dup_map;    /* 记录重复的函数符号 */


void *image_alloc(const char *path, uintptr_t load_base)
{
    image *img = new image(path, load_base);
    images.push_back(img);
    return img;
}

void image_collect_symbol(void *p_image, const char *name, uintptr_t addr)
{
    image *img = (image *)p_image;
    std::string symbol(name);
    if (img->symbols.count(symbol) == 1) {
        if (dup_map[symbol].size() == 0) {
            dup_map[symbol].push_back(img->symbols[symbol]);
            fprintf(stderr, "duplicate symbols: %s, %lu\n", name, addr);
        }
        dup_map[symbol].push_back(addr);
    }
    img->symbols[symbol] = addr;
}

uintptr_t image_get_symbol_by_name(void *p_image, const char *name)
{
    image *img = (image *)p_image;
    std::string symbol(name);
    if (img->symbols.count(symbol) == 0) {
        return -1UL;
    }
    return img->symbols[symbol];
}


void print_collected_symbols(void)
{
    for (image *img : images) {
        printf("Image: %s\n", img->path.c_str());
        for (auto &p : img->symbols) {
            printf("%p: %s\n", (void *)p.second, p.first.c_str());
        }
        printf("Routines count: %lu\n", img->symbols.size());
    }

    if (dup_map.empty()) {
        printf("\nNo duplicated symbols. :)\n");
        return;
    }
    printf("\nDuplicated symbols:\n");
    for (auto &p : dup_map) {
        printf("%s: ", p.first.c_str());
        for (uintptr_t addr : p.second) {
            printf("%p, ", (void *)addr);
        }
        printf("\n");
    }
}

