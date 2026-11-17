# V02-02 工作包2：主用Target完整构建验证

- 执行日期：2026-11-17；版本记录日期：2026-11-18。
- 基线：493ab126082c3c3fc38500d58c57c49877ecc395，加当时未提交工作区（含Keil本机配置），不是只构建Git导出树。
- 最终范围由用户明确调整为只使用Linko 07x Project；Linko 07x Project_ac6不再使用。
- 结果：主用Target编译/链接PASS；原工作区全量不变检查有IDE元数据例外，不能称整个任务无例外通过。

## 环境与真实集成

uVision5.41.0.0/MDK5.41，实际ArmClang6.22。Device=LKS32MC071CBT8，Linko.LKS07x.1.2.0；PDSC/工程/Scatter均Flash128KiB、RAM12KiB。C99/Cortex-M0编译，未替换或升级SDK/HAL。

主Target实际宏为BENCH_TEST_CMP_FAIL。三个真实保护模块各加入一次，与main及真实SDK共同全量编译链接；Mock目录/源文件没有进入MCU工程或最终MAP。A5源码保留编译。主用Target最终34个C+1汇编、35个OBJ，AXF/MAP/HEX均生成，退出码0，0错误0警告。

源工作区Keil宏/工程配置是用户本地未提交状态，仍保留在本地，未随版本管理提交。因此仅checkout基线原工程（默认A5宏）不等于重现本次CMP_FAIL入口构建；重现需按记录在隔离副本选择BENCH_TEST_CMP_FAIL，不能据本报告擅自变更原工作区。各保护模块当时源代码与基线一致。

CMSIS工程声明package5.4.0/CORE5.1.2，但当前缺5.4.0，编译依赖实际使用已安装ARM.CMSIS6.1.0；本轮未安装或升级包。此版本锁定差异保留，不能声称验证5.4.0。

## 内存

| Code | RO | RW | ZI | Flash | RAM |
|---:|---:|---:|---:|---:|---:|
| 3824 B | 224 B | 4 B | 2012 B | 4052 B/128KiB | 2016 B/12KiB |

主TargetMAP确认ROM4052、RW2016。不据此推导运行时栈峰值。AXF/HEX文件大小不等同占用。

## 隔离与完整性

当时433个工作区文件（包括未提交、未跟踪、忽略文件）全部复制至两个独立副本；每个副本初始逐文件SHA核对通过。输出/中间文件均指向副本，最终AXF/HEX/OBJ使用新V02_02_Objects；MAP和启动LST按实际.lnp输出至副本Listings，未采用旧复制产物冒充结果。

主TargetRebuild两次：首次辅助输出目录脚本有转义错误，仍在副本Objects构建成功；修正后在全新目录完整重建为最终归档。第二Target经用户允许6.16→6.22后尝试一次，30个SDK07x缺头错误、汇总0警告、未链接；日志有1条warning标签的include上下文，原样保留在完整报告。用户随后排除此Target，不继续修配置，不写成双TargetPASS。

源码、SDK、生产头文件、main、A5及uvprojx/uvoptx和Git状态均未变；但最终观察到原目录被忽略的uvguix布局内容变化，以及build_log.htm和.dep仅mtime变化。布局差异为工具栏/编辑器行列；原因无法确定，不归因也不擅自恢复。严格“原目录所有字节/时间戳不变”无法签字通过。具体差异留在独立目录audit中。

## 产物指纹（最终轮）

AXF/HEX/MAP/日志以及所有OBJ的完整路径、大小、时间和SHA256均在本机artifact-manifest.json。二进制、原始日志、原始本机项目副本不加入Git。

| 文件 | 大小B | SHA256 |
|---|---:|---|
| Build_Output.log | 1439 | d13ccab1b2dc6423a28e749ea520dcaad251dd456aa467242f3a4e033d789fe4 |
| LK_StdPeriph.axf | 75588 | 3fd7698576363a3dbfbfbec29d980765d82d1c47b17e829a41152322cf47d05b |
| LK_StdPeriph.hex | 11465 | 836ad703c2c3c2114500c0a52561c84759837c402b1e237bce3ae33f8c8b611d |
| LK_StdPeriph.map | 194483 | 5937f8a2e9c75b4401bc49835091e30167e060fe7fd52c45988cbdd8469cd7d5 |

当时本地uvprojx（未随提交）的SHA256：567d14c66fa12cf23c73a5e9a10cd5fb335034a9cb4f5c31c1b382dc7a061ec8。

## 边界与归档

真实config_valid/raw_valid/hardware_qualified仍0、FAIL1_EN=0、RUN/MOE开启BLOCKED、P0-P4 NOT RUN。无烧录、硬件上电、功率板操作；不因HEX生成判硬件/CBC/OCP通过。CMSIS依赖锁定与硬件前置条件另行审核。

本机完整归档根目录为当前协作工作目录的_isolated_build/V02-02_20261117_233617，含V02-02_Keil_Build_Report.md、artifact-manifest.json、原始构建日志和audit。任务执行时未提交/推送；当前版本管理为用户后来明确授权。不自行推进下一硬件工作包。
