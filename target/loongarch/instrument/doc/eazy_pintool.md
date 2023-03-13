# 简单易懂的pintool教程

{{TODO 添加编译命令演示，添加插桩工具的输出示例}}

根据Intel Pin插桩框架API编写的插桩工具称为pintool。Pin的插桩API十分友好，可以简单高效地编写插桩工具pintool。

在本教程中你将学习到如何编写以下插桩工具。

- 指令插桩：打印执行的指令
- 基本块插桩：更高效的指令计数
- 系统调用插桩：starce
- 函数插桩：监控所有的malloc调用

## 基本概念介绍

本插桩框架以JIT的方式进行动态插桩，每当程序将要执行指令时，插桩框架会获取这些指令进行插桩，并执行新生成的插桩后代码，程序的原始代码不会被执行。插桩后的代码会被缓存，因此应用程序再次执行到相同指令时不必重新插桩。

插桩对象主要有：
- 指令（INS）
- 基本块（BBL）：由多条连续的指令组成，只有最后一条指令为控制转移指令，如跳转指令、系统调用等。
- 轨迹（TRACE）：由多个基本块组成。本插桩框架不支持轨迹插桩，为了兼容Pin API，每个轨迹里只有一个基本块。
- 系统调用（SYSCALL）
- 函数（Routine）

编写插桩工具需要编写“插桩函数”和“分析函数”：
- 插桩函数：用于声明如何插入分析函数的调用。
- 分析函数：要被插入到应用程序中的函数调用，在程序运行过程中被调用。

## 指令插桩

### 示例

下面代码展示了如何进行指令插桩，它在每一条指令执行前插入对函数 `show_ins(UINT64 pc, UINT32 opcode)` 的调用。

```c
#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"

// 用于统计指令执行总数
static UINT64 icount = 0;

// 分析函数：指令计数并打印执行的指令
static VOID show_ins(UINT64 pc, UINT32 opcode)
{
    ++icount;

    // 打印指令
    char msg[128];
    Ins ins;
    la_disasm(opcode, &ins);
    sprint_ins(&ins, msg);
    fprintf(stderr, "[thread %d] %lu\tpc: 0x%lx\t%s\n", PIN_ThreadId(), icount, pc, msg);
}

// 插桩函数：每当遇到一条指令时进行插桩
static VOID Instruction(INS ins, VOID* v)
{
    // 在每条指令前插入一个 show_ins 的函数调用，并传递参数pc和opcode
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)show_ins, IARG_UINT64, ins->pc, IARG_UINT64, ins->opcode, IARG_END);
}

// 程序结束时输出统计的总指令数
static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "Total Ins Count: %lu\n", icount);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    // 注册插桩函数
    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
```

说明：
1. 在 `main` 函数中 `INS_AddInstrumentFunction(Instruction, 0)` 注册插桩函数 `Instruction`，每当遇到一条指令时都会调用该函数。
2. 在 `main` 函数中 `PIN_AddFiniFunction(Fini, 0)` 注册回调函数 `Fini`，会在应用程序执行结束时调用。
3. `Instruction` 函数调用 `INS_InsertCall` 向该指令INS插入函数调用 `show_ins`，并传递pc和opcode两个参数。
4. 应用程序运行时，在每条指令前都会调用 `show_ins` 打印指令信息，并增加指令计数器 `icount`。
5. 应用程序结束时，调用 `Fini` 函数输出总共执行的指令数。

## 基本块插桩

在每条指令前插入一个函数调用，会导致程序性能大幅下降。实际上许多的运行信息都可以通过只对基本块插桩来收集。

### 示例

下面的代码展示了如何对基本块插桩，统计指令和基本块的执行数量，并在程序结束时输出统计结果，以及每个基本块的平均指令数。

```c
#include "pintool.h"
#include "../ins_inspection.h"

static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;
static UINT64 icount = 0;

static VOID bbl_statistic(uint64_t ins_nr) {
    ++bbl_exec_nr;
    ins_exec_nr += ins_nr;
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
	// 插入分析函数调用
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)bbl_statistic, IARG_UINT64, BBL_NumIns(bbl), IARG_END);
        // 内联插桩，利用该插桩API可以直接对icount变量增值
	// BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &icount, BBL_NumIns(bbl), false);
    }
}

// 输出统计结果
static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INSs/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
```
说明：
1. 在 `main` 函数中通过 `TRACE_AddInstrumentFunction` 注册轨迹插桩函数 `Trace`，每当遇到一个轨迹时被调用。
2. `Trace` 函数遍历轨迹中的每个基本块BBL，并在每个基本块前插入分析函数 `bbl_statistic` 的调用。
3. `bbl_statistic` 函数记录已经执行的指令数和基本块数目。
4. 程序结束时调用 `Fini` 打印统计结果。

**优化：内联插桩**

对于像指令计数这样的“向某个内存地址中的变量增加固定数值”的操作，本插桩框架提供了更高效的插桩API `BBL_InsertInlineAdd` 和 `INS_InsertInlineAdd`，它们通过插入几条指令来完成这个操作，而不是插入一个函数调用。`BBL_InsertInlineAdd` 的最后一个参数是否对增值操作加锁，这用于在多线程程序中保证统计信息的正确性，但同时也会导致性能的略微下降。在本例中，你可以使用该API代替 `BBL_InsertCall` 实现更高效的插桩。


## 系统调用插桩

你可以在系统调用前后插桩，并在分析函数中获取系统调用号、系统调用的参数以及返回值。

注：
1. 目前 `CONTEXT *ctx` 变量功能并不完善，请不要直接修改内部数据；
2. `SYSCALL_STANDARD std` 只是为了兼容 Pin API，并无实际意义。

### 示例

下面的代码展示了如何在系统调用前后插入函数调用，实现和strace类似的功能。

```c
#include "pintool.h"
#include "../ins_inspection.h"

static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctx, SYSCALL_STANDARD std, VOID *v)
{
    int syscall_nr = PIN_GetSyscallNumber(ctx, std);
    fprintf(stderr, "[tid %u] %s (a0-a6: 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)\n",
            threadIndex,
            syscall_name(syscall_nr), 
            PIN_GetSyscallArgument(ctx, std, 0),
            PIN_GetSyscallArgument(ctx, std, 1),
            PIN_GetSyscallArgument(ctx, std, 2),
            PIN_GetSyscallArgument(ctx, std, 3),
            PIN_GetSyscallArgument(ctx, std, 4),
            PIN_GetSyscallArgument(ctx, std, 5),
            PIN_GetSyscallArgument(ctx, std, 6));
}

static VOID syscall_exit(THREADID threadIndex, CONTEXT *ctx, SYSCALL_STANDARD std, VOID *v)
{
    fprintf(stderr, "[tid %u] = %ld\n",
            threadIndex, PIN_GetSyscallReturn(ctx, std));
}

static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    /* syscall trace */
    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);

    return 0;
}
```

## 函数插桩

插桩框架支持在函数的入口和出口插桩，允许获取函数调用的参数以及返回值。

注意：
1. 函数插桩需要用到应用程序的符号信息，无法对已经 `strip` 了的程序进行函数插桩。
2. 本插桩框架实现函数插桩的方式与Pin不同，不对函数原始代码进行修改。
3. 目前函数插桩只进行过简单测试。

### 示例

下面代码对 `malloc` 和 `free` 函数进行插桩。

```c
#include "pintool.h"
#include "../symbol.h"
#include "../../instrument/elf/symbol.h"

static VOID Arg1Before(CHAR* name, ADDRINT size) {
    fprintf(stderr, "%s(%lx)\n", name, size);
}
 
static VOID MallocAfter(ADDRINT ret) {
    fprintf(stderr, " returns %lx\n", ret);
}

static VOID Image(IMG img, VOID* v)
{
    /* Malloc */
    RTN mallocRtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, "malloc", IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
        RTN_Close(mallocRtn);
    }
 
    /* Free */
    RTN freeRtn = RTN_FindByName(img, "free");
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, "free", IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(freeRtn);
    }

}
 

static INT32 Usage(void)
{
    return -1;
}
 

int main(int argc, char* argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();
 
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
 
    PIN_StartProgram();
 
    return 0;
}
```

说明：
1. 函数插桩必须在 `main` 函数中调用 `PIN_InitSymbols` 声明启用符号解析。
2. `IMG_AddInstrumentFunction` 注册插桩函数 `Image`，对每个映像IMG进行插桩（映像指ELF文件，包括可执行程序以及动态链接库）
3. `Image` 函数中在 `malloc` 函数的入口和出口插入分析函数调用，在 `free` 函数的入口插入分析函数调用。在函数入口打印参数，在函数出口打印返回值。

## 结语

至此你已基本了解了插桩工具的编写方式，可以模仿着编写自己的插桩工具。

如果你想了解更多的插桩API，可以查看Intel Pin的用户手册，我们实现了大部分常用的API，具体的实现情况见[文档](./pin_api_implementation.md)。

