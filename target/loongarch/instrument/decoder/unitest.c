/* Unit Test for la_disasm & la_assemble 
 * Build: gcc *.c -o unitest
 */
#include "disasm.h"
#include "assemble.h"
#include "la_print.h"
#include <stdio.h>

typedef struct testcase {
    int opcode;
    char *str;
} testcase;

/* objdump -> vim :s/\v([0-9a-z])+\s+(.*)/{0x\1, "\2"}, */
testcase cases[] = {
    {0x00150089, "move    $r9,$r4"},
    {0x1c0000e4, "pcaddu12i       $r4,7(0x7)"},
    {0x03ab100c, "ori     $r12,$r0,0xac4"},
    {0x1600000c, "lu32i.d $r12,0"},
    {0x0300018c, "lu52i.d $r12,$r12,0"},
    {0x380c3084, "ldx.d   $r4,$r4,$r12"},
    {0x28c00065, "ld.d    $r5,$r3,0"},
    {0x02c02066, "addi.d  $r6,$r3,8(0x8)"},
    {0x00451063, "srli.d  $r3,$r3,0x4"},
    {0x00411063, "slli.d  $r3,$r3,0x4"},
    {0x1c0000e7, "pcaddu12i       $r7,7(0x7)"},
    {0x03aa600c, "ori     $r12,$r0,0xa98"},
    {0x1600000c, "lu32i.d $r12,0"},
    {0x0300018c, "lu52i.d $r12,$r12,0"},
    {0x380c30e7, "ldx.d   $r7,$r7,$r12"},
    {0x1c0000e8, "pcaddu12i       $r8,7(0x7)"},
    {0x03a9900c, "ori     $r12,$r0,0xa64"},
    {0x1600000c, "lu32i.d $r12,0"},
    {0x0300018c, "lu52i.d $r12,$r12,0"},
    {0x380c3108, "ldx.d   $r8,$r8,$r12"},
    {0x0015006a, "move    $r10,$r3"},
    {0x1c0000e1, "pcaddu12i       $r1,7(0x7)"},
    {0x03a9700c, "ori     $r12,$r0,0xa5c"},
    {0x1600000c, "lu32i.d $r12,0"},
    {0x0300018c, "lu52i.d $r12,$r12,0"},
    {0x380c3021, "ldx.d   $r1,$r1,$r12"},
    {0x4c000021, "jirl    $r1,$r1,0"},
    {0x1c0000e1, "pcaddu12i       $r1,7(0x7)"},
    {0x03a9300c, "ori     $r12,$r0,0xa4c"},
    {0x1600000c, "lu32i.d $r12,0"},
    {0x0300018c, "lu52i.d $r12,$r12,0"},
    {0x380c3021, "ldx.d   $r1,$r1,$r12"},
    {0x4c000021, "jirl    $r1,$r1,0"},
    {0x03400000, "andi    $r0,$r0,0x0"},
};

int main() {
    for (int i = 0; i < sizeof(cases)/sizeof(testcase); ++i) {
        printf("======\n");
        uint32_t opcode = cases[i].opcode;
        printf("origin: %08x\t", opcode);
        printf("%s\n", cases[i].str);

        Ins ins;
        la_disasm(opcode, &ins);
        uint32_t asm_insn = la_assemble(&ins);
        printf("disasm: %08x\t", asm_insn);
        print_ins(&ins);
    }
}

