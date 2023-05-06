#include "reg.h"
#include "../instrument/regs.h"
#include "../instrument/util/error.h"

bool REG_is_gpr(REG REG)
{
    return REG_GPR_BASE <= REG && REG <= REG_GPR_LAST;
}

bool REG_is_fpr(REG REG)
{
    return REG_FPR_BASE <= REG && REG <= REG_FPR_LAST;
}

int REG_to_gpr(REG REG)
{
    lsassert(REG_is_gpr(REG));
    return REG - REG_GPR_BASE;
}

int REG_to_fpr(REG REG)
{
    lsassert(REG_is_fpr(REG));
    return REG - REG_FPR_BASE;
}

REG gpr_to_REG(int gpr)
{
    lsassert(reg_zero <= gpr && gpr <= reg_s8);
    return gpr + REG_GPR_BASE;
}

REG fpr_to_REG(int fpr)
{
    lsassert(reg_f0 <= fpr && fpr <= reg_f31);
    return fpr + REG_FPR_BASE;

}
