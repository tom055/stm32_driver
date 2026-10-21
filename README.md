# STM32F103C8T6 标准库工程

使用 VS Code、EIDE 和 ARM GCC，芯片配置为 Cortex-M3、64KB Flash、20KB SRAM。

## 本机工具

- ARM GNU Toolchain 15.2.Rel1（ZIP 解压版）：`C:/licheng/arm-gcc/15.2.rel1`
- SEGGER J-Link 9.86（当前用户安装）：`C:/Users/ChengLi/SEGGER/JLink_V986`
- VS Code 扩展：`cl.eide`、`marus25.cortex-debug`。

工具路径已配置到工作区文件和 `.vscode/settings.json`，无需修改系统 PATH。换电脑时需调整这两处路径。

代码提示已配置 ARM GCC 和 `windows-gcc-arm` 模式，并接入 EIDE 生成的 `build/Debug/compile_commands.json`。如果 EIDE 没有提供某个文件的编辑器配置，C/C++ 扩展会使用编译数据库；尚未进入数据库的新文件使用项目的头文件路径和 STM32 宏作为基础配置。增加源文件、包含路径或预处理宏后，在 EIDE 中构建一次以更新编译数据库。

## 构建与调试

1. 打开 `stm32f1xx_gcc-quickstart.code-workspace`。
2. 在 EIDE 面板中选择 `Debug`，点击“构建”或“重新构建”。
3. 输出位于 `build/Debug/stm32f1xx_gcc.elf`、`.hex`、`.bin` 和 `.map`。
4. 接好 J-Link 和开发板的 SWD 后，可使用 EIDE 烧录按钮，或在“运行和调试”中选择 `jlink` 并按 F5。

手动调试前先构建；F5 本身不自动构建。当前下载器配置为 J-Link、SWD、8000kHz。若实际使用 ST-Link 或 CMSIS-DAP，需要相应切换下载和调试配置。

## Wokwi 仿真

项目根目录已配置 `wokwi.toml` 和 `diagram.json`。虚拟电路使用 STM32 Blue Pill 和 I²C LCD1602，LCD 的 7 位地址为 `0x27`。

| STM32 Blue Pill | LCD1602 |
| --- | --- |
| PB6 | SCL |
| PB7 | SDA |
| 5V | VCC |
| GND | GND |

1. 先在 EIDE 中构建，生成 `build/Debug/stm32f1xx_gcc.hex` 和 `.elf`。
2. 执行 `Wokwi: Start Simulator`，当前示例在 LCD 第二行第一列开始显示 `World`。
3. 如需断点调试，先执行 `Wokwi: Start Simulator and Wait for Debugger`，再选择 `Wokwi GDB` 并按 F5。

`jlink` 配置用于真机，`Wokwi GDB` 用于仿真。Wokwi 授权需通过插件提供的许可证流程完成。

## LCD 驱动

驱动位于 `src/bsp/lcd.c` 和 `src/bsp/lcd.h`，使用标准库配置 I2C1，经过 PCF8574 以四位模式控制 LCD。

```c
lcd_init();               // 初始化 I²C 和 LCD
lcd_send(0, 0, "Hello"); // 第一行第一列
lcd_send(1, 0, "World"); // 第二行第一列
```

`line` 为 `0` 或 `1`，`col` 为 `0`～`15`。请由调用方保证行列有效，并控制字符串长度；当前接口不会自动截断或换行。`lcd_write_char()` 从当前写入位置发送单个字符。

LCD 指令参考 [HD44780U 数据手册](https://cdn-shop.adafruit.com/datasheets/HD44780.pdf)，第 24 页为指令表，第 46 页为四位接口初始化流程。

## 板级约定

- 时钟按外部 **8MHz 晶振**配置，PLL 倍频后系统时钟为 72MHz。
- 当前示例使用 **I2C1：PB6/PB7** 驱动 LCD1602。
- 已加入该型号适用的标准库驱动；DAC、CEC、FSMC、SDIO 的源文件保留在磁盘，但排除编译。
- 当前未实现串口标准输入输出重定向，`printf` 不会自动输出到串口。`nosys.specs` 在新工具链中可能产生未实现系统调用的链接警告。

编译通过仅验证软件构建；实际晶振、LCD 接线、下载器连接和板上运行需连接硬件确认。
