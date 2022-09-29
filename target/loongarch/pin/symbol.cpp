#include "symbol.h"
#include <stdio.h>

#include "target/loongarch/instrument/elf/symbol.h"


RTN RTN_FindByName(IMG img, const CHAR *name)
{
    uintptr_t addr = image_get_symbol_by_name(img, name);
    RTN rtn = { .name = name, .addr = addr};
    return rtn;
}

BOOL RTN_Valid(RTN x)
{
    return (x.addr != -1UL);
}

/* do nothing ... */
VOID RTN_Open(RTN rtn)
{
}

VOID RTN_Close(RTN rtn)
{
}


#include <unordered_map>
#include <vector>

static std::unordered_map<uintptr_t, std::vector<ANALYSIS_CALL>> rtn_cbs;

VOID RTN_save_cb(uintptr_t addr, ANALYSIS_CALL cb)
{
    rtn_cbs[addr].push_back(cb);
}

ANALYSIS_CALL *RTN_get_cbs(uintptr_t addr, int *n)
{
    if (rtn_cbs.count(addr) == 0) {
        *n = 0;
        return NULL;
    }
    *n = rtn_cbs[addr].size();
    return rtn_cbs[addr].data();
}
