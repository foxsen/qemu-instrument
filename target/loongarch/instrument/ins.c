#include "ins.h"
#include "util/error.h"
#include <stdlib.h>
#include "regs.h"

#include "translate.h"

/* TODO: 应该放在 lsenv->tr_data */
/* Note: tr_data can only be used after tr_init()! */
__thread TRANSLATION_DATA tr_data;

void tr_init(void *tb)
{
    TRANSLATION_DATA *t = &tr_data;
    t->curr_tb = tb;
    t->is_jmp = TRANS_NEXT;
    t->jmp_ins[0] = NULL;
    t->jmp_ins[1] = NULL;

    /* allocate ins_array when called at the first time */
    if (t->ins_array == NULL) {
        t->max_ins_nr = MAX_INS_NR;
        t->ins_array = (Ins *)calloc(t->max_ins_nr, sizeof(Ins));
    }
    t->cur_ins_nr = 0;

    t->list_ins_nr = 0;
    t->first_ins = NULL;
    t->last_ins = NULL;
}

void tr_fini(void)
{}


Ins *ins_alloc(void)
{
    TRANSLATION_DATA *t = &tr_data;
    lsassertm(t->cur_ins_nr < t->max_ins_nr, "too many ins\n");

    Ins *ins = t->ins_array + t->cur_ins_nr;
    t->cur_ins_nr++;

    /* memset(ins, 0, sizeof(*ins)); */
    ins->opnd_count = 0;
    ins->prev = NULL;
    ins->next = NULL;
    return ins;
}

Ins *ins_copy(Ins *old)
{
    Ins *ins = ins_alloc();
    ins->op = old->op;
    ins->opnd_count = old->opnd_count;
    ins->opnd[0].val = old->opnd[0].val;
    ins->opnd[1].val = old->opnd[1].val;
    ins->opnd[2].val = old->opnd[2].val;
    ins->opnd[3].val = old->opnd[3].val;
    return ins;
}

void ins_append(Ins *ins)
{
    TRANSLATION_DATA *t = &tr_data;
    Ins *last_ins = t->last_ins;

    if (last_ins != NULL) {
        lsassert(t->first_ins != NULL);
        last_ins->next = ins;
        ins->prev = last_ins;
        ins->next = NULL;
        t->last_ins = ins;
    } else {
        lsassert(t->first_ins == NULL);
        ins->prev = NULL;
        ins->next = NULL;
        t->first_ins= ins;
        t->last_ins = ins;
    }
    t->list_ins_nr++;
}

void ins_append_load_imm64(int reg, uint64_t imm)
{
    uint32_t lo32 = imm & 0xffffffff;
    uint32_t hi32 = imm >> 32;
    /* ins_append_2(LISA_LU12I_W, reg, lo32 >> 12); */
    /* ins_append_3(LISA_ORI, reg, reg, lo32 & 0xfff); */
    /* ins_append_2(LISA_LU32I_D, reg, hi32 & 0xfffff); */
    /* ins_append_3(LISA_LU52I_D, reg, reg, hi32 >> 20); */

    /* Reduce generated ins number */
    /* The following code is copied from LAT */

    char sign_bitmap, all0_bitmap, allf_bitmap;
    char sign_extension_by_previous;
    char extension_by_previous_hi, extension_by_previous_lo;

    /*
     * We divided the imm64 into the following four parts and used
     * bitmap to represent the status of each part.
     *
     * | imm64      | bitmap |       |
     * | ---------- | ------ | ----- |
     * | imm[11:0]  | bit0   | part0 |
     * | imm[31:12] | bit1   | part1 |
     * | imm[51:32] | bit2   | part2 |
     * | imm[63:52] | bit3   | part3 |
     *
     */
    sign_bitmap = (((hi32 & 0x80000000) != 0) << 3) |
                  (((hi32 & 0x00080000) != 0) << 2) |
                  (((lo32 & 0x80000000) != 0) << 1) |
                  ((lo32 & 0x00000800) != 0);
    /* 1 means that this part equals 0 */
    all0_bitmap = (((hi32 & 0xfff00000) == 0) << 3) |
                  (((hi32 & 0x000fffff) == 0) << 2) |
                  (((lo32 & 0xfffff000) == 0) << 1) |
                  ((lo32 & 0x00000fff) == 0);
    /* 1 means that this part equals -1 */
    allf_bitmap = (((hi32 & 0xfff00000) == 0xfff00000) << 3) |
                  (((hi32 & 0x000fffff) == 0x000fffff) << 2) |
                  (((lo32 & 0xfffff000) == 0xfffff000) << 1) |
                  ((lo32 & 0x00000fff) == 0x00000fff);

    /*
     * Except part0, if the part is equal to 0 or -1, it may be a
     * sign extension of the previous part.
     *
     * bit0 equals 1 means part0 equals 0.
     */
    sign_extension_by_previous = (all0_bitmap ^ allf_bitmap) &
                                 (all0_bitmap ^ (sign_bitmap << 1));
    extension_by_previous_lo = sign_extension_by_previous & 0x3;
    extension_by_previous_hi = sign_extension_by_previous >> 2;

    /* the fast path */
    if (sign_extension_by_previous == 0x7) {
        /*
         * xxx 00000 00000 000
         * part0, part1 and part2 = 0, we just load part3
         */
        ins_append_3(LISA_LU52I_D, reg, reg_zero, hi32 >> 20);
        return;
    }

    /*
     * load lo32
     */
    switch (extension_by_previous_lo) {
    case 0x0:
        /*
         * lo32: 00000/xxxxx xxx
         */
        if ((all0_bitmap & 0x3) == 0x2) {
            /*
             * lo32: 00000 xxx
             * part1 = 0, we just load part0
             */
            ins_append_3(LISA_ORI, reg, reg_zero, lo32 & 0xfff);
        } else {
            /*
             * lo32: xxxxx xxx
             */
            ins_append_2(LISA_LU12I_W, reg, lo32 >> 12);
            ins_append_3(LISA_ORI, reg, reg, lo32 & 0xfff);
        }
        break;
    case 0x1:
        /*
         * lo32: xxxxx 000
         */
        ins_append_2(LISA_LU12I_W, reg, lo32 >> 12);
        break;
    case 0x2:
        /*
         * lo32: 00000/fffff xxx
         * part0 != 0, part1 is sign extension of the part0
         */
        ins_append_3(LISA_ADDI_W, reg, reg_zero, lo32 & 0xfff);
        break;
    case 0x3:
        /*
         * lo32: 00000 000
         */
        ins_append_3(LISA_OR, reg, reg_zero, reg_zero);
        break;
    }

    /*
     * load hi32:
     * now, the lower 32 bits of the imm64 are loaded, and the
     * higher 32 bits of the opnd2 are signed extensions of the
     * lower.
     */
    switch (extension_by_previous_hi) {
    case 0x0:
        /*
         * hi32: xxx xxxxx
         */
        ins_append_2(LISA_LU32I_D, reg, hi32 & 0xfffff);
        ins_append_3(LISA_LU52I_D, reg, reg, hi32 >> 20);
        break;
    case 0x1:
        /*
         * hi32: xxx 00000/fffff
         */
        ins_append_3(LISA_LU52I_D, reg, reg, hi32 >> 20);
        break;
    case 0x2:
        /*
         * hi32: 000/fff xxxxx
         */
        ins_append_2(LISA_LU32I_D, reg, hi32 & 0xfffff);
        break;
    case 0x3:
        /*
         * hi32: 00000000/ffffffff
         */
        break;
    }
}

void INS_append_ins(INS INS, Ins *ins)
{
    if (INS->first_ins == NULL) {
        lsassert(INS->last_ins == NULL && INS->len == 0);
        INS->first_ins = ins;
        INS->last_ins = ins;
        INS->len = 1;
        tr_data.list_ins_nr++;
        return;
    }

    lsassert(INS->last_ins != NULL && INS->len != 0);
    /* 更新ins */
    ins->prev = INS->last_ins;
    INS->last_ins->next = ins;
    /* 更新INS */
    INS->last_ins = ins;
    INS->len++;
    /* 更新tr_data */
    tr_data.list_ins_nr++;
}

/* 只将元素从链表中移除，但是仍然可以通过它可以访问到前后元素 */
/* BUG prompt: 调用此接口时，必须保证ins在INS内 */
void INS_remove_ins(INS INS, Ins *ins)
{
    if (INS->first_ins == INS->last_ins) {
        lsassert(ins == INS->first_ins);
        INS->first_ins = NULL;
        INS->last_ins = NULL;
    } else if (INS->first_ins == ins) {
        INS->first_ins = ins->next;
    } else if (INS->last_ins == ins) {
        INS->last_ins = ins->prev;
    }

    if (ins->prev != NULL)  ins->prev->next = ins->next;
    if (ins->next != NULL)  ins->next->prev = ins->prev;

    INS->len--;
    tr_data.list_ins_nr--;
}

void INS_insert_ins_before(INS INS, Ins *old, Ins *ins)
{
    if (INS->first_ins == old) {
        INS->first_ins = ins;
    }

    ins->prev = old->prev;
    ins->next = old;
    if (old->prev != NULL) {
        old->prev->next = ins;
    }
    old->prev = ins;

    INS->len++;
    tr_data.list_ins_nr++;
}

void INS_insert_ins_after(INS INS, Ins *old, Ins *ins)
{
    if (INS->last_ins == old) {
        INS->last_ins = ins;
    }

    ins->prev = old;
    ins->next = old->next;
    if (old->next != NULL) {
        old->next->prev = ins;
    }
    old->next = ins;

    INS->len++;
    tr_data.list_ins_nr++;
}

void INS_load_imm64_before(INS INS, Ins *ins, int reg, uint64_t imm)
{
    uint32_t lo32 = imm & 0xffffffff;
    uint32_t hi32 = imm >> 32;
    /* INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU12I_W, reg, lo32 >> 12)); */
    /* INS_insert_ins_before(INS, ins, ins_create_3(LISA_ORI, reg, reg, lo32 & 0xfff)); */
    /* INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU32I_D, reg, hi32 & 0xfffff)); */
    /* INS_insert_ins_before(INS, ins, ins_create_3(LISA_LU52I_D, reg, reg, hi32 >> 20)); */

    /* Reduce the number of ins to load an immediate */
    /* The following code is copied from LAT */

    char sign_bitmap, all0_bitmap, allf_bitmap;
    char sign_extension_by_previous;
    char extension_by_previous_hi, extension_by_previous_lo;

    /*
     * We divided the imm64 into the following four parts and used
     * bitmap to represent the status of each part.
     *
     * | imm64      | bitmap |       |
     * | ---------- | ------ | ----- |
     * | imm[11:0]  | bit0   | part0 |
     * | imm[31:12] | bit1   | part1 |
     * | imm[51:32] | bit2   | part2 |
     * | imm[63:52] | bit3   | part3 |
     *
     */
    sign_bitmap = (((hi32 & 0x80000000) != 0) << 3) |
                  (((hi32 & 0x00080000) != 0) << 2) |
                  (((lo32 & 0x80000000) != 0) << 1) |
                  ((lo32 & 0x00000800) != 0);
    /* 1 means that this part equals 0 */
    all0_bitmap = (((hi32 & 0xfff00000) == 0) << 3) |
                  (((hi32 & 0x000fffff) == 0) << 2) |
                  (((lo32 & 0xfffff000) == 0) << 1) |
                  ((lo32 & 0x00000fff) == 0);
    /* 1 means that this part equals -1 */
    allf_bitmap = (((hi32 & 0xfff00000) == 0xfff00000) << 3) |
                  (((hi32 & 0x000fffff) == 0x000fffff) << 2) |
                  (((lo32 & 0xfffff000) == 0xfffff000) << 1) |
                  ((lo32 & 0x00000fff) == 0x00000fff);

    /*
     * Except part0, if the part is equal to 0 or -1, it may be a
     * sign extension of the previous part.
     *
     * bit0 equals 1 means part0 equals 0.
     */
    sign_extension_by_previous = (all0_bitmap ^ allf_bitmap) &
                                 (all0_bitmap ^ (sign_bitmap << 1));
    extension_by_previous_lo = sign_extension_by_previous & 0x3;
    extension_by_previous_hi = sign_extension_by_previous >> 2;

    /* the fast path */
    if (sign_extension_by_previous == 0x7) {
        /*
         * xxx 00000 00000 000
         * part0, part1 and part2 = 0, we just load part3
         */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LU52I_D, reg, reg_zero, hi32 >> 20));
        return;
    }

    /*
     * load lo32
     */
    switch (extension_by_previous_lo) {
    case 0x0:
        /*
         * lo32: 00000/xxxxx xxx
         */
        if ((all0_bitmap & 0x3) == 0x2) {
            /*
             * lo32: 00000 xxx
             * part1 = 0, we just load part0
             */
            INS_insert_ins_before(INS, ins, ins_create_3(LISA_ORI, reg, reg_zero, lo32 & 0xfff));
        } else {
            /*
             * lo32: xxxxx xxx
             */
            INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU12I_W, reg, lo32 >> 12));
            INS_insert_ins_before(INS, ins, ins_create_3(LISA_ORI, reg, reg, lo32 & 0xfff));
        }
        break;
    case 0x1:
        /*
         * lo32: xxxxx 000
         */
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU12I_W, reg, lo32 >> 12));
        break;
    case 0x2:
        /*
         * lo32: 00000/fffff xxx
         * part0 != 0, part1 is sign extension of the part0
         */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_ADDI_W, reg, reg_zero, lo32 & 0xfff));
        break;
    case 0x3:
        /*
         * lo32: 00000 000
         */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_OR, reg, reg_zero, reg_zero));
        break;
    }

    /*
     * load hi32:
     * now, the lower 32 bits of the imm64 are loaded, and the
     * higher 32 bits of the opnd2 are signed extensions of the
     * lower.
     */
    switch (extension_by_previous_hi) {
    case 0x0:
        /*
         * hi32: xxx xxxxx
         */
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU32I_D, reg, hi32 & 0xfffff));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LU52I_D, reg, reg, hi32 >> 20));
        break;
    case 0x1:
        /*
         * hi32: xxx 00000/fffff
         */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LU52I_D, reg, reg, hi32 >> 20));
        break;
    case 0x2:
        /*
         * hi32: 000/fff xxxxx
         */
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_LU32I_D, reg, hi32 & 0xfffff));
        break;
    case 0x3:
        /*
         * hi32: 00000000/ffffffff
         */
        break;
    }
}

Ins *ins_create_0(IR2_OPCODE op)
{
    Ins *ins = ins_alloc();
    ins->op = op;
    ins->opnd_count = 0;
    return ins;
}

Ins *ins_create_1(IR2_OPCODE op, int opnd0)
{
    Ins *ins = ins_alloc();
    ins->op = op;
    ins->opnd_count = 1;
    ins->opnd[0].val = opnd0;
    return ins;
}

Ins *ins_create_2(IR2_OPCODE op, int opnd0, int opnd1)
{
    Ins *ins = ins_alloc();
    ins->op = op;
    ins->opnd_count = 2;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    return ins;
}

Ins *ins_create_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2)
{
    Ins *ins = ins_alloc();
    ins->op = op;
    ins->opnd_count = 3;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    ins->opnd[2].val = opnd2;
    return ins;
}

Ins *ins_create_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3)
{
    Ins *ins = ins_alloc();
    ins->op = op;
    ins->opnd_count = 4;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    ins->opnd[2].val = opnd2;
    ins->opnd[3].val = opnd3;
    return ins;
}


Ins *ins_append_0(IR2_OPCODE op)
{
    Ins *ins = ins_create_0(op);
    ins_append(ins);
    return ins;
}

Ins *ins_append_1(IR2_OPCODE op, int opnd0)
{
    Ins *ins = ins_create_1(op, opnd0);
    ins_append(ins);
    return ins;
}

Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1)
{
    Ins *ins = ins_create_2(op, opnd0, opnd1);
    ins_append(ins);
    return ins;
}

Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2)
{
    Ins *ins = ins_create_3(op, opnd0, opnd1, opnd2);
    ins_append(ins);
    return ins;
}

Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3)
{
    Ins *ins = ins_create_4(op, opnd0, opnd1, opnd2, opnd3);
    ins_append(ins);
    return ins;
}

#include "qemu/bitops.h"
Ins *ins_nop(void)
{
    return ins_create_3(LISA_ORI, reg_zero, reg_zero, 0);
}

Ins *ins_b(long offs26)
{
    /* PC = PC + (offs26 << 2) */
    lsassert(offs26 == sextract64(offs26, 0, 26));
    return ins_create_1(LISA_B, offs26);
}

Ins *ins_pcaddi(int rd, long offs20)
{
    /* $rd = PC + (offs20 << 2) */
    lsassert(offs20 == sextract64(offs20, 0, 20));
    return ins_create_2(LISA_PCADDI, rd, offs20);
}

