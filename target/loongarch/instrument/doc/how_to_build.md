# 运行

## 编译
```bash
mkdir build
cd build
# 编译插桩的版本
'../configure' '--target-list=loongarch64-linux-user' '--enable-debug' --extra-cflags='-DCONFIG_LMJ -DCONFIG_LMJ_DEBUG' --extra-ldflags='-export-dynamic'
# 编译tcg版本
#'../configure' '--target-list=loongarch64-linux-user' '--enable-debug'
ninja
```
说明：
- `--extra-cflags`: 不用tcg，而是用我的翻译器
- `--extra-ldflags`：导出了qemu的全部符号(用于pintool插件)

## 运行
```bash
build/qemu-loongarch64 {{cmd}}
```

options
```
-showtrans: show translation
-fullregs: ld/st every reg in instruction
-d nochain: disable tb_link
-noibtc: not use jmp_cache for JIRL
-no-debug-log: not print debug log
-lmj_debug: 为了和qemu对比trace，保持环境变量等因素一致
-t <pintoll>: use pintool to instrument 
```
## pintool
```bash
# 编译pintool
mkdir -p build/target/loongarch/pin/pintools 
cd build/target/loongarch/pin/pintools 
ln -s /home/loongson/code/qemu/target/loongarch/pin/pintools/makefile  makefile
make

# 使用
build/qemu-loongarch64 -t {{path-to-pintool}} {{cmd}}
```


