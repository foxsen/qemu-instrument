#include "symbol.h"
#include <stdio.h>
#include "../instrument/elf/symbol.h"
#include "../instrument/util/error.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <limits>

const CHAR *RTN_FindNameByAddress(ADDRINT address)
{
    symbol_info *sym = get_symbol_by_pc(address);
    if (sym == NULL || sym->addr == 0) {
        return "";
    }
    return sym->name;
}

RTN RTN_FindByName(IMG img, const CHAR *name)
{
    symbol_info *sym = image_get_symbol_by_name((image *)img, name);
    if (sym == NULL || sym->addr == 0) {
        return NULL;
    }

    if (is_symbol_name_dupcalited(name)) {
        lswarn("symbol %s is duplicated in image.\n", name);
    }
    if (sym->size == 0) {
        /* FIXME! can we instrument a function whose size is zero? */
        lswarn("Find a symbol with zero size: name: %s, addr: 0x%lx, size: 0\n", name, sym->addr);
    }

    RTN rtn = RTN_alloc(name, sym->addr, sym->size);
    return rtn;
}

BOOL RTN_Valid(RTN x)
{
    return (x != NULL && x->addr != 0);
    /* FIXME maybe size should not be zero? */
    /* return (x != NULL && x->addr != 0 && x->size != 0); */
}

/* do nothing ... */
VOID RTN_Open(RTN rtn)
{
}

VOID RTN_Close(RTN rtn)
{
}



/* === 下面为内部实现所需接口 === */

static auto compare_rtn = [](const RTN lhs, const RTN rhs)
{
    if (lhs->addr != rhs->addr) {
        return lhs->addr < rhs->addr;
    } else {
        return lhs->size < rhs->size;
    }
};

/* 函数插桩
 * 1. RTN_FindByName 会创建一个 RTN
 * 2. RTN_InsertCall 向 RTN 注册 callbacks（目前是通过 rtn_entry_cbs 和 rtn_exit_cbs 维护 callbacks）
 * BUG: 以相同的image的name多次调用 RTN_FindByName 会创建多个相同的 RTN，导致 rtn_exit_cbs 里有多个相同的 rtn
 */
static std::unordered_map<uintptr_t, std::vector<ANALYSIS_CALL>> rtn_entry_cbs;
static std::map<RTN, std::vector<ANALYSIS_CALL>, decltype(compare_rtn)> rtn_exit_cbs(compare_rtn);

VOID RTN_add_entry_cb(RTN rtn, ANALYSIS_CALL *cb)
{
    rtn_entry_cbs[rtn->addr].emplace_back(*cb);
}

ANALYSIS_CALL *RTN_get_entry_cbs(uintptr_t pc, int *cnt)
{
    if (rtn_entry_cbs.count(pc) == 0) {
        *cnt = 0;
        return NULL;
    }
    *cnt = rtn_entry_cbs[pc].size();
    return rtn_entry_cbs[pc].data();
}

VOID RTN_add_exit_cb(RTN rtn, ANALYSIS_CALL *cb)
{
    rtn_exit_cbs[rtn].emplace_back(*cb);
}

ANALYSIS_CALL *RTN_get_exit_cbs(uintptr_t pc, int *cnt)
{
    /* find a rtn which contains the pc */
    /* FIXME 目前假设所有rtn不重叠，因此pc只出现在一个rnt的范围中 */
    struct pin_rtn key = { .addr = pc, .size = std::numeric_limits<uint64_t>::max() };
    auto iter = rtn_exit_cbs.upper_bound(&key);
    if (iter != rtn_exit_cbs.begin()) {
        RTN rtn = prev(iter)->first;
        if (rtn->addr <= pc && pc < rtn->addr + rtn->size) {
            *cnt = rtn_exit_cbs[rtn].size();
            return rtn_exit_cbs[rtn].data();
        }
    }

    *cnt = 0;
    return NULL;
}
