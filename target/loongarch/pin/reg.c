#include "reg.h"
#include "../instrument/regs.h"
#include "../instrument/util/error.h"

/* transform PIN's REG to internal gpr */
int REG_to_gpr(REG REG)
{
    lsassert(REG_GR_BASE <= REG && REG <= REG_GR_LAST);
    return REG - REG_GR_BASE;
}

REG gpr_to_REG(int gpr)
{
    lsassert(reg_zero <= gpr && gpr <= reg_s8);
    return gpr + REG_GR_BASE;
}

