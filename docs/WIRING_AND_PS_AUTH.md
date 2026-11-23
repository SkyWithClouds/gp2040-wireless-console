# 接线、刷机与 PS 认证器

接收盒中 **Pico 的 Micro-USB 是主机口**，接 PC 或游戏机。**USB-A 母座是认证器口**。NOLOGO #2 的 Type-C 用于单独刷机。把两块板和所有 USB 线断电后再焊线。

## 接收 Pico 到认证器 USB-A 母座

| Pico 物理脚 | 信号 | USB-A 母座 |
|---|---|---|
| Pin 1 · GP0 | D+ | D+，串联 22 Ω |
| Pin 2 · GP1 | D− | D−，串联 22 Ω |
| Pin 3 · GND | 地 | GND |
| Pin 40 · VBUS | 主机 USB 5 V | VBUS |

母座焊脚从背面看会镜像。按实际母座引脚图核对，通电后先测母座 VBUS 对 GND，再插认证器。上游 USB Host 顺序选 **D+ / D−**，5 V 使能 GPIO 留空，Pin 40 直接供电。

## 接收 Pico 到 NOLOGO #2

| Pico 物理脚 | NOLOGO 丝印 | 排针 | 方向与用途 |
|---|---|---|---|
| Pin 24 · GP18 | P0.20 | NP1 Pin 7 | Pico → #2，SPI0 SCK |
| Pin 25 · GP19 | P0.17 | NP1 Pin 6 | Pico → #2，MOSI 交付确认及灯态 |
| Pin 21 · GP16 | P0.22 | NP1 Pin 8 | #2 → Pico，MISO 输入快照 |
| Pin 22 · GP17 | P0.24 | NP1 Pin 9 | Pico → #2，CS 低有效 |
| Pin 26 · GP20 | P1.00 | NP1 Pin 10 | #2 → Pico，READY 高有效 |
| Pin 23 · GND | GND | NP1 Pin 4 或 5 | 两板共地 |
| Pin 36 · **3V3 OUT** | **BAT** | NP2 Pin 12 | 接收盒工作时给 #2 供电，建议这根线可拔 |

**供电红线：严禁将 5 V、VBUS 或 VSYS 接到 NOLOGO 的 BAT 或任何 GPIO。** 使用上述接法时让 Pico 通过原生 USB 取电，#2 不装电池。给任一板刷机前先拔主机线、认证器、3V3→BAT 线并断开板间连接，再单独连接待刷板。尤其避免接着 3V3→BAT 时再给 #2 插 Type-C，使充电电路与 Pico 稳压输出相连。

## 分别刷机

1. #2 单独接 PC，快速复位两次打开 UF2 引导盘。两块已核对样板的 `INFO_UF2.TXT` 均为 Bootloader 0.6.0、Model nice!nano、SoftDevice S140 6.1.1。拖入 `01-rx-bridge-nologo-26000.uf2`，等重启后拔线。应用起址为 `0x26000`，Family ID 为 `0xADA52840`。
2. 官方 Pico 单独按住 BOOTSEL 接 PC，看到 RPI-RP2 后松手，拖入 `02-rx-gp2040ce-pico.uf2`。这是 **接收端** Pico。发射侧两块板继续使用原固件。
3. 两板断电后按表接回。已有 GP2040-CE 配置的 Pico 可能保留旧 GPIO 或外设映射。先打开 Web Configurator 核对；需要恢复本板默认值时，先备份所需设置，再使用网页的重置设置功能。

## 没有本地按键，怎样进网页

在 **断电时** 用跳线将接收 Pico 的 **Pin 29 / GP22** 短接 **Pin 28 / GND**，用 Pico 的 Micro-USB 数据线接 PC。GP22 是开机维护脚，固件内部上拉；正常使用留空。进入配置模式后取下跳线，打开 [192.168.7.1](http://192.168.7.1)。完成设置后保存并重新启动。BOOTSEL 进入的是 UF2 刷机盘。

## 配置认证与方向

1. 在 **Configuration → Peripheral Mapping** 查看 USB 外设。启用 USB，D+ 为 GP0、顺序为 D+ 后接 D−，对应 GP1。保存。
2. 在 **Settings → Input Mode Settings** 选择 **PS4** 或 **PS5**。在该模式的 **Authentication Settings** 选 **Host USB**。两种模式的出厂认证方式均已设置。Xbox One 模式使用上游兼容的 Xbox 认证器；XInput 模式另有自己的认证设置。普通 Windows XInput 验收可直接使用默认值。
3. 在游戏手柄设置里把 **D-Pad Mode** 设为 **D-Pad**，**SOCD Cleaning Mode** 选 **Off**，对应 Bypass。无线 Addon 在本地输入处理结束后才注入最终快照。部分主机模式的方向帽只能编码单一方向或斜向，最终以该主机报告能表示的状态为准。
4. 保存后拔掉 Pico，撤下维护跳线。将与目标主机和游戏兼容的实体认证器插入 USB-A 母座，再用 Micro-USB 把 Pico 接到主机。分别记录 MagicBoots 的具体版本或 PXN N5 型号、主机系统版本和游戏名。PS5 的上游实现按街机摇杆类设备工作，验收请选支持该类设备的原生 PS5 游戏。PS4 游戏在 PS5 上运行应另记一项。

主机能识别接收盒后，继续测试超过 8 分钟，并在游戏中实际操作。若能短时操作后断开，优先核对模式、Host USB、D+/D−、母座供电以及认证器在同一主机上的有线表现。要排除无线问题，先在 Windows 的 XInput 模式完成短按、长按和断发射端电源测试。
