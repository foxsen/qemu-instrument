# 代码结构
主要分为三个部分：
- LA指令集信息：target/loongarch/instrument/decoder
- 翻译部分：target/loongarch/instrument
- 插桩部分：target/loongarch/pin

## LA指令集
- ins.h: 定义了 `Ins` 结构体用于表示一条指令，并实现了检查获取指令信息的一些相关接口、
- assemble.h: 将 `Ins` 生成为汇编代码
- disasm.h: 从汇编代码解析生成 `Ins`
- ir2.h: LA指令的相关信息

## 翻译部分
核心代码在：
- 主流程：target/loongarch/instrument/instrument.c
- 翻译：target/loongarch/instrument/translate.c

翻译时用到的两个模块：
- 指令：ins.c
- 寄存器：reg.c

### 整体函数调用流程
```
tb_gen_code()
    tr_init(tb);
    la_decode(cpu, tb, max_insns)
		while(1)
			uint32_t opcode = read_opcode(cs, pc);
			la_disasm(opcode, origin_ins);
			INS_translate(cs, ins);
			INS_instrument(ins);
    la_relocation(cpu, tb->tc.ptr);
    gen_code_size = la_encode(tcg_ctx, gen_code_buf) * 4;
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
// 翻译函数，因为la to la只需要翻译PCADDI/B/JIRL/SYSCALL，因此全写在一个函数里了
int INS_translate(CPUState *cs, INS INS);
// single-step时用于给每个tb结尾加出口
void INS_append_exit(INS INS, uint32_t index);

// 上下文切换，熟悉latx应该很亲切
extern uint64_t context_switch_bt_to_native;
extern uint64_t context_switch_native_to_bt_ret_0;
extern uint64_t context_switch_native_to_bt;
int la_gen_prologue(CPUState *cs, TCGContext *tcg_ctx);
int la_gen_epilogue(CPUState *cs, TCGContext *tcg_ctx);
```

## ins_list结构
1. 全局变量 `tr_data` 保存了翻译时的信息，其中最主要的是 `ins_list`。
2. 翻译生成的指令放到 `ins_list` 链表中，最后由 `la_encode` 生成最终的二进制代码。

### INS, BBL, TRACE
为了方便插桩，不直接对 `ins_list` 操作：
1. 一条LA ins，被翻译/插桩后变为多条指令，放在一个 `INS` 结构体中
2. 一个TB的多条 `INS` 构成一个 `BBL`
3. 为了兼容PIN的API，`BBL`构成`TRACE`
4. 整体的层级：TRACE->BBL->INS
5. `la_encode` 根据 `tr_data.trace` 遍历指令，就生成了最终代码
6. `la_encode` 也可以直接根据 `ins_list` 来生成二进制

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

另外也可以直接对 `ins_list` 操作
```c
Ins *ins_append_0(IR2_OPCODE op);
Ins *ins_append_1(IR2_OPCODE op, int opnd0);
Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);
```


## 寄存器
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

