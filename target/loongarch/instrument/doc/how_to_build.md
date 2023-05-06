# 编译和使用

## 编译

### 编译QEMU

```sh
# At the root directory of this repository
mkdir build
cd build
# 编译插桩的版本
../configure --target-list=loongarch64-linux-user --enable-debug --extra-cflags=-DCONFIG_LMJ -DCONFIG_LMJ_DEBUG --extra-ldflags=-export-dynamic
# 编译原始QEMU TCG翻译版本
#../configure --target-list=loongarch64-linux-user --enable-debug
ninja # or use make
```

说明：
- 需要提前安装编译QEMU所需软件
- `--extra-cflags`: 添加宏定义，表示不要使用QEMU TCG翻译，而是启用插桩
- `--extra-ldflags`：导出了qemu的全部符号(用于pintool插件)

### 编译插桩工具pintool

插桩工具pintool放在target/loongarch/pin/pintools目录下。
新增的pintool请添加到makefile中的NAMES变量中，pintool编译后得到添加lib前缀的.so文件。

```sh
mkdir -p build/target/loongarch/pin/pintools 
cd build/target/loongarch/pin/pintools 
ln -s /home/[...]/qemu/target/loongarch/pin/pintools/makefile makefile
make
```

## 使用

```sh
./build/qemu-loongarch64 -t [path-to-pintool] [CMD]
# 示例：插桩统计指令执行数
./build/qemu-loongarch64 -t ~/[...]/qemu/build/target/loongarch/pin/pintools/libinline_add.so [CMD]
```

使用`-t`指定插桩工具，目前不支持同时使用多个pintool，且无法向pintool传递参数。

一些可选的参数：（要放在[CMD]之前！）
```
-t <pintool>: use pintool to instrument 
-showtrans: show translation
-debug-log: print debug log
-d nochain: disable tb_link
-noibtc: not use jmp_cache for JIRL
-lmj-debug: debug时为了和qemu对比trace，控制环境变量等因素一致
```

