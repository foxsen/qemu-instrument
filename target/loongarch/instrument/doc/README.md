# XXX README

XXX 是一个开源的龙架构平台（LoongArch）的用户态的动态二进制插桩框架。

该软件基于QEMU开发，并且实现了插桩框架
[Intel Pin](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html)
的（部分）插桩API，你可以按照pintool的写法编写插桩工具，在LoongArch平台上对指令、基本块、系统调用、函数进行插桩。

本插桩框架基于QEMU开发，移除了TCG代码生成组件，重新实现了代码生成部分，相比QEMU有更高的执行效率（尤其是浮点运算）。缺点是其只能在LoongArch平台上运行，而不能像QEMU一样跨平台运行。（目前市面上可购买到的支持LoongArch指令集的CPU只有3A5000）

如图是分别使用Pin、XXX、QEMU-LoongArch64在不插桩情况下相对原生运行 SPEC 2006 的执行效率。

{TODO SPEC2006 性能对比图}

~~此时你也许在想直接把Intel给出的pintool示例代码直接搬过来使用，这样做大概率是不行的，因为本框架目前没有实现 `KNOB` 部分的API，而这在Intel的pintool示例代码中被大量使用。如果你想要使用Intel Pin给出的示例代码，你需要修改这些代码，但可能工作量并不是很大。另一方面，本软件目前实现的Pin API有限，加上指令集架构的差异，也会导致无法直接使用Pin的官方代码示例。~~

如果你不太了解如何编写插桩工具，这里有一份 [简单易懂的pintool教程](./eazy_pintool.md)。

## 当前状态

该软件目前仍处在开发初期阶段，基本的插桩功能均已实现，可以运行 SPEC 2006 及一些小型程序，更复杂的大型程序还待测试。

[Pin API 的实现情况](./pin_api_implementation.md)。由于指令集架构存在差异，部分API未实现。

{{TODO 设计实现文档和图 }}

## 快速上手

1. 编译插桩框架

```sh
mkdir build
cd build
'../configure' '--target-list=loongarch64-linux-user' '--enable-debug' --extra-cflags='-DCONFIG_LMJ -DCONFIG_LMJ_DEBUG' --extra-ldflags='-export-dynamic'
make # or ninja
```

2. 编译插桩工具

```sh
# 在 `target/loongarch/pin/pintools` 目录下有部分插桩工具的源代码示例。
# 编译好的插桩工具放在 `build/target/loongarch/pin/pintools` 目录下。
mkdir -p build/target/loongarch/pin/pintools
cd build/target/loongarch/pin/pintools 
ln -s {{ABSOLUT_PATH_TO_REPOSITORY}}/target/loongarch/pin/pintools/makefile  makefile
make
```

3. 运行应用程序并插桩

```sh
cd build
# 你可以使用 `-t` 指定插桩工具。
./qemu-loongarch64 -no-debug-log [-t pintool_path] {{CMD}}
```

如果你想要了解pintool的编写，请参考 Intel Pin 的用户手册。

更详细的[编译文档](./how_to_build.md)

## 插桩工具编写注意事项

- 插桩位置 `IPOINT` 目前只支持 `IPOINT_BEFORE`，即只支持在某条指令INS或某个基本块BBL前插入函数调用（这已经能应付大多数插桩场景）。（对于系统调用和函数插桩，允许使用 `IPONT_AFTER` 在系统调用返回和函数返回时插桩）
- 插桩函数没有内联优化，因此对每条指令INS都插桩会导致程序性能大幅下降，请尽量采用对基本块BBL插桩的方式收集信息。
- 建议使用 `BBL_InsertInlineAdd` 和 `INS_InsertInlineAdd` 这两个插桩API，它们的效果是令某个内存地址中的值加上指定的值，该插桩接口不会进行函数调用，可以编写出高性能的插桩工具。


## Contribution

{{ TODO }}
