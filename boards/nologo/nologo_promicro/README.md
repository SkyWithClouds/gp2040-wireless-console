# NOLOGO ProMicro nRF52840 · 两块实板记录

使用者在 #1 和 #2 的 UF2 引导盘上分别核对了以下字段。

| INFO_UF2.TXT 字段 | 两块板的实测值 |
|---|---|
| Bootloader | UF2 Bootloader 0.6.0 |
| Model | nice!nano |
| SoftDevice | S140 version 6.1.1 |

已确认应用起址 `0x26000`、UF2 Family ID `0xADA52840` 与这两块板匹配。此前 Windows XInput 枚举与 1P 灯验收来自原无线桥工程。本接收盒使用相同板定义；NOLOGO #2 在应用模式下运行无线和 SPIS，主机 USB 改由接收 Pico 提供。双板及主机侧的实板验收见[清单](../../../docs/CHECKLIST.md)。
