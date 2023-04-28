# 代码结构
主要分为三个部分：
- 指令解析：target/loongarch/instrument/decoder
- 指令翻译：target/loongarch/instrument
- 指令插桩：target/loongarch/pin

## 指令解析(decoder)
- ins.h: 定义了表示一条LA指令的 `Ins` 结构体。并提供了一系列获取指令信息（指令类型、操作数类型、跳转目标地址）函数。
- disasm.h: 解析二进制，生成 `Ins`
- assemble.h: 将 `Ins` 汇编生成为二进制代码
- ir2.h: 指令编码格式等基本信息
- la_print.h：将指令、操作数转换为人类可读的字符串

指令编码格式等数据保存在如下表中：

```
// 建议通过 `ins.h` 中提供的指令检查API获取这些信息
lisa_format_table[]     // LoongArch指令编码格式
ir2_opnd_type_table[]   // 操作数(rd, rj, rk) -> 操作数类型(GPR/FPR/IMM)
bit_field_table[]       // 操作数(rd, rk, rk) -> 操作数所在位域
ir2_opnd_type_table[]   // 指令中各操作数对寄存器的读写情况（不完整）
```

## 翻译部分
核心代码在：
- 主流程：target/loongarch/instrument/instrument.c
- 翻译：target/loongarch/instrument/translate.c

### 翻译流程(有简化)
```
tb_gen_code()
    tr_init(tb);
    la_decode(cpu, tb, max_insns)
		while(1)
			uint32_t opcode = read_opcode(cs, pc);
			la_disasm(opcode, origin_ins);
			INS_translate(cs, INS);
			INS_instrument(INS);
    la_relocation(cpu, tb->tc.ptr);
    la_encode(tcg_ctx, gen_code_buf);
    tr_fini();
```

### 主要的函数
```c
/* === instrument.c === */
// 反汇编 + 翻译 + 插桩
int la_decode(CPUState *cs, TranslationBlock *tb, int max_insns);
// 跳转指令重定位
void la_relocation(CPUState *cs, const void *code_buf_rx);
// 生成汇编代码
int la_encode(TCGContext *tcg_ctx, void *code_buf);

/* === translate.c === */
// 指令翻译
int INS_translate(CPUState *cs, INS INS);
// 给TB结尾添加返回到QEMU的出口
void INS_append_exit(INS INS, uint32_t index);

// QEMU和应用程序的上下文切换
extern uint64_t context_switch_bt_to_native;
extern uint64_t context_switch_native_to_bt_ret_0;
extern uint64_t context_switch_native_to_bt;
int la_gen_prologue(CPUState *cs, TCGContext *tcg_ctx);
int la_gen_epilogue(CPUState *cs, TCGContext *tcg_ctx);
```

### ins_list结构
1. 全局变量 `tr_data` 保存了翻译时的信息，其中最主要的是 `ins_list`。
2. 翻译生成的指令序列放到 `ins_list` 链表中，最后由 `la_encode` 生成最终的二进制代码。

### INS, BBL, TRACE
为了方便插桩，不直接对 `ins_list` 操作：
1. 一条LA ins，被翻译/插桩后变为多条指令，放在一个 `INS` 结构体中
2. 多条 `INS` 构成一个 `BBL` 基本块
3. 为了兼容PIN的API，`BBL`构成`TRACE`
4. `la_encode` 根据 `tr_data.trace` 遍历指令链表，生成最终二进制
5. `la_encode` 也可以直接根据 `ins_list` 来生成二进制

### 插入指令
示例
```c
// 在ins之前插入几条指令：load立即数next_pc到itemp_ra
INS_load_imm64_before(INS, ins, itemp_ra, next_pc);
// 在ins之前插入指令：LISA_ST_D x,x,x
INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, itemp_ra, reg_env, env_offset_of_gpr(cs, reg_ra)));
```

注：连续的对同一个ins用`INS_insert_ins_before`，可以保证指令被按照顺序插入。

主要用到的API
```c
void INS_insert_ins_before(INS INS, Ins *old, Ins *ins);
void INS_load_imm64_before(INS INS, Ins *ins, int reg, uint64_t imm);
Ins *ins_create_0(IR2_OPCODE op);
Ins *ins_create_1(IR2_OPCODE op, int opnd0);
Ins *ins_create_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_create_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_create_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);
```

另外也可以直接对 `ins_list` 操作，在链表结尾添加指令
```c
Ins *ins_append_0(IR2_OPCODE op);
Ins *ins_append_1(IR2_OPCODE op, int opnd0);
Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);
```


### 寄存器
```c
/* regs.h */
// 直接映射的寄存器的接口
bool gpr_is_mapped(int gpr);
int mapped_gpr(int gpr);

// 临时寄存器分配的接口
int reg_alloc_itemp(void);
void reg_free_itemp(int itemp);

// 申请临时寄存器
// 翻译时，若需要临时寄存器，需要主动申请和释放
int itemp = reg_alloc_itemp();
INS_insert_ins_before(INS, ins, ins_create_3(LISA_ADD_D, itemp_tb, reg_env, itemp));
reg_free_itemp(itemp)
```

