#ifndef _REGS_H_
#define _REGS_H_

#include <stdbool.h>

/* LA registers */
/* GPR */
#define reg_invalid -1
#define reg_zero    0
#define reg_ra      1
#define reg_tp      2
#define reg_sp      3
#define reg_a0      4
#define reg_a1      5
#define reg_a2      6
#define reg_a3      7
#define reg_a4      8
#define reg_a5      9
#define reg_a6      10
#define reg_a7      11
#define reg_t0      12
#define reg_t1      13
#define reg_t2      14
#define reg_t3      15
#define reg_t4      16
#define reg_t5      17
#define reg_t6      18
#define reg_t7      19
#define reg_t8      20
#define reg_x       21
#define reg_fp      22
#define reg_s0      23
#define reg_s1      24
#define reg_s2      25
#define reg_s3      26
#define reg_s4      27
#define reg_s5      28
#define reg_s6      29
#define reg_s7      30
#define reg_s8      31
#define reg_end     32

#define reg_syscall_nr      reg_a7
#define reg_syscall_arg0    reg_a0 
#define reg_syscall_arg1    reg_a1 
#define reg_syscall_arg2    reg_a2 
#define reg_syscall_arg3    reg_a3 
#define reg_syscall_arg4    reg_a4 
#define reg_syscall_arg5    reg_a5 
#define reg_syscall_arg6    reg_a6 
#define reg_syscall_ret     reg_a0 

/* save native info in bt context */
#define reg_env reg_x
/* TODO maybe reg_target can also be used as a temp reg */
#define reg_target reg_a7
/* NOTE: reg_ret saves the return value: ( tb | jmp_slot )
 * consider the context_switch_native_to_bt process, it should be mapped to a reg that:
 * 1. is a temp reg, so its value will not be spilled to env
 * 2. can not be a callee-saved reg, whose value will be covered when fill from stack */
#define reg_ret reg_a6

/* FCSR0 */
#define reg_fcsr    0

/* FCC[8] */
#define reg_fcc0    0
#define reg_fcc1    1
#define reg_fcc2    2
#define reg_fcc3    3
#define reg_fcc4    4
#define reg_fcc5    5
#define reg_fcc6    6
#define reg_fcc7    7

/* FPR[32] */
#define reg_f0      0
#define reg_f1      1
#define reg_f2      2
#define reg_f3      3
#define reg_f4      4
#define reg_f5      5
#define reg_f6      6
#define reg_f7      7
#define reg_f8      8
#define reg_f9      9
#define reg_f10     10
#define reg_f11     11
#define reg_f12     12
#define reg_f13     13
#define reg_f14     14
#define reg_f15     15
#define reg_f16     16
#define reg_f17     17
#define reg_f18     18
#define reg_f19     19
#define reg_f20     20
#define reg_f21     21
#define reg_f22     22
#define reg_f23     23
#define reg_f24     24
#define reg_f25     25
#define reg_f26     26
#define reg_f27     27
#define reg_f28     28
#define reg_f29     29
#define reg_f30     30
#define reg_f31     31

/* not used in user-mode */
#define reg_scr0    0
#define reg_scr1    1
#define reg_scr2    2
#define reg_scr3    3

bool gpr_is_mapped(int gpr);
int mapped_gpr(int gpr);
bool fpr_is_mapped(int fpr);
int mapped_fpr(int fpr);


int reg_alloc_itemp(void);
void reg_free_itemp(int itemp_gpr);
void reg_debug_check_itemp_all_free(void);
int reg_map_gpr_to_itemp(int gpr);
void reg_unmap_gpr_to_itemp(int gpr);


#endif

