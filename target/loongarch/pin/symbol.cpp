#include "symbol.h"
#include <stdio.h>
#include "target/loongarch/instrument/elf/symbol.h"

#include <vector>
#include <unordered_map>
#include <map>

RTN RTN_FindByName(IMG img, const CHAR *name)
{
    symbol_info sym = image_get_symbol_by_name(img, name);
    if (sym.addr == 0) {
        return NULL;
    }

    if (sym.size == 0) {
        /* FIXME Can we instrument a function whose size is zero? */
        fprintf(stderr, "find symbol with zero size: name: %s, addr: 0x%lx, size: 0\n", name, sym.addr);
    }
    RTN rtn = RTN_alloc(name, sym.addr, sym.size);
    return rtn;
}

BOOL RTN_Valid(RTN x)
{
    return (x != NULL && x->addr != 0);
    /* FIXME maybe this one is better? */
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

static auto cmp_rtn = [](RTN lhs, RTN rhs)
{
    if (lhs->addr != rhs->addr) {
        return lhs->addr < rhs->addr;
    } else {
        return lhs->size < rhs->size;
    }
};

static std::unordered_map<uintptr_t, std::vector<ANALYSIS_CALL>> rtn_entry_cbs;
static std::map<RTN, std::vector<ANALYSIS_CALL>, decltype(cmp_rtn)> rtn_exit_cbs(cmp_rtn);

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
    /* 1. find rtn contains pc */
    /* FIXME 目前假设所有rtn不重叠，因此pc只出现在一个rnt的范围中 */
    /* TODO 现在是在注册了cb的rtn中搜索符合pc的rtn，也可以写一个函数：在所有的符号信息中查找rtn */
    RTN find_routine = NULL;
    struct pin_rtn key = { .addr = pc, .size = 0 };
    auto iter = rtn_exit_cbs.lower_bound(&key);
    if (iter != rtn_exit_cbs.end()) {
        RTN rtn = iter->first;
        if (rtn->addr <= pc && pc < rtn->addr + rtn->size) {
            find_routine = rtn;
        }
    }
    if (!find_routine && iter != rtn_exit_cbs.begin()) {
        iter = prev(iter);
        RTN rtn = iter->first;
        if (rtn->addr <= pc && pc < rtn->addr + rtn->size) {
            find_routine = rtn;
        }
    }

    if (!find_routine) {
        *cnt = 0;
        return NULL;
    }

    /* 2. return cbs */
    *cnt = rtn_exit_cbs[find_routine].size();
    return rtn_exit_cbs[find_routine].data();
}
