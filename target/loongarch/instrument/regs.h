#ifndef _REGS_H_
#define _REGS_H_

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


/* save native info in bt context*/
#define reg_code_ptr reg_x
#define reg_target reg_fp
#define reg_env reg_s8

#define INVALID_TEMP    -1
#define ITEMP0          0
#define ITEMP1          1
#define ITEMP2          2
#define ITEMP3          3
#define ITEMP4          4
#define ITEMP5          5
#define ITEMP6          6
#define ITEMP7          7
#define ITEMP8          8

extern const int reg_gpr_map[];
int gpr_is_mapped(int gpr);
int reg_alloc_gpr(int gpr);

int reg_alloc_itemp(void);
void reg_free_itemp(int itemp);
void reg_debug_itemp_all_free(void);
int reg_map_gpr_to_itemp(int gpr);
void reg_unmap_gpr_to_itemp(int gpr);

void set_mapped_gpr_dirty(int gpr);
void set_mapped_gpr_freeable(int gpr);
int gpr_is_mapped_to_itemp(int gpr);
int itemp_is_dirty(int itemp);
int gpr_mapped_to_itemp(int gpr);
int find_and_free_a_freeable_itemp(void);
void debug_print_itemp_status(void);
int need_save_an_itemp_gpr(void);

#endif

