# GP2040-CE 无线接收盒

把已有的 GP2040-CE Hitbox 无线发射组接到另一台主机。手柄侧继续使用原来的成品 Hitbox、Pico 和 NOLOGO #1 固件。主机侧换成两块板组成的接收盒。

## 三仓 2×2 生态

| 发射端 ↓ / 接收端 → | PC 迷你接收器 | 主机认证接收盒 |
|---|---|---|
| 外挂成品 Hitbox | pico-host + radio-tx → usb-rx | pico-host + radio-tx → rx-bridge + gp2040-rx |
| 单板 18 键 Hitbox | hitbox-nrf52840 → usb-rx | hitbox-nrf52840 → rx-bridge + gp2040-rx |

[外挂发射端与 PC 接收器](https://github.com/SkyWithClouds/gp2040-wireless) · [单板 Hitbox 与网页](https://github.com/SkyWithClouds/nrf52840-wireless-hitbox) · [主机接收盒](https://github.com/SkyWithClouds/gp2040-wireless-console)

## 无线匹配与双打（v1.1 预览）

出厂网络 ID 为 `B74632DA`，信道 40，速率 2 Mbps。槽位 1 在本机使用 Pipe 1，其基础地址与前缀 C5 保留旧 Pipe 0 的空口地址，原版 v1.0 板可继续配合使用。Slot 2–4 需要升级对应发射板和 PC 接收器。玩家槽位决定无线输入路由到哪个 XInput 接口；Windows 和游戏自行分配控制器序号。

1. 在桌面 Chrome / Edge 用 USB 数据线连接要配置的无线板，打开[WebHID 配置台](https://skywithclouds.github.io/nrf52840-wireless-hitbox/)。修改外挂发射板或主机盒无线桥前，断开板间供电和信号线，单独给 NOLOGO 插 USB。
2. 在「无线匹配与接收器管理」生成一次网络 ID，给同一组设备填写相同的八位 ID 和信道。信道 2–80 对应 2402–2480 MHz。不同比赛台可分别选 ID；同频设备仍共享空口时间。
3. PC 接收器默认启用 2 个槽位，可设 1–4。两台发射端分别选 1、2。主机盒固定使用槽位 1，允许修改 ID、信道。每台保存后等待回读，再完全断电重启；单板 Hitbox 拔线进入无线即加载已保存的设置。
4. PC 接收器网页每约 150 ms 显示每槽状态、收到的有效帧、漏收心跳、重复输入和超时次数，计数从开机开始。漏收心跳按同一会话的心跳编号缺口累计，真实射频丢包率需借助空口测量。
5. 双打实测时让两人同时点按，按住 1P 后断开 1P 电源，检查 1P 释放、2P 仍可操作。交换角色重复，再测修改信道、恢复出厂参数和断电保存。USB 枚举、射频吞吐及实际输入延迟待实板验证。


```text
原发射组 ── 2.4 GHz ESB ──► NOLOGO #2 ── SPI ──► 接收 Pico ── Micro-USB ──► 主机
                                                      │
                                             USB-A 母座接实体认证器
```

| 刷写对象 | 固件 | 职责 |
|---|---|---|
| 接收盒 NOLOGO ProMicro nRF52840 #2 | `01-rx-bridge-nologo-26000.uf2` | ESB 接收、会话与队列、SPIS 从机、确认及玩家灯 ACK Payload |
| 接收盒官方 Raspberry Pi Pico（RP2040） | `02-rx-gp2040ce-pico.uf2` | GP2040-CE、无线输入 Addon、主机 USB、认证器透传、Web Configurator |

**兼容基线**为发射端提交 `1967a24c27c0dbc7fd60f2adc065cb4a09c3a980`。无线地址、RF_CH 40、2 Mbps、24 字节帧、64 位会话号、CRC-8/SMBUS、8 帧窗口及 50 ms 看门狗沿用 v1.0。原发射端的两份状态机源码直接参与本仓库的兼容测试。[原发射工程](https://github.com/SkyWithClouds/gp2040-wireless/tree/1967a24c27c0dbc7fd60f2adc065cb4a09c3a980)

**主机模式**沿用官方 GP2040-CE v0.7.12。这个版本的 Web 菜单有 16 种输入模式，包括 PS4、PS5、XInput、Xbox One、Original Xbox、Switch、Switch Pro、Generic HID、键盘及七种其他模式。出厂默认 XInput，PS4 和 PS5 的认证方式均预设为 Host USB。PS5 使用上游街机摇杆兼容路径，支持范围取决于具体游戏、主机版本和实体认证器。[官方版本](https://github.com/OpenStickCommunity/GP2040-CE/releases/tag/v0.7.12) · [上游输入模式说明](https://gp2040-ce.info/input-mode-configuration/)

**验证状态**：协议向量、未修改发射端到接收 Pico 的状态机联测已通过。三个 GitHub Actions 任务已跑绿，两份固件均已产出。工作流逐块检查 UF2 与对应链接二进制、芯片家族、Flash 地址和复位向量。真实射频、主机枚举、PS/Xbox 认证、持续连接时间和端到端延迟待实板验收。[构建与固件下载](https://github.com/SkyWithClouds/gp2040-wireless-console/actions/workflows/build.yml)

- 初次装机请按[接线、供电与 PS 认证配置](docs/WIRING_AND_PS_AUTH.md)操作。
- 测试时按[明日实板验收清单](docs/CHECKLIST.md)记录各主机的结果。
- 开发者可运行 `scripts/test.sh`，并在 GitHub 的 **Actions → Console receiver CI → Run workflow** 重建固件。两个 Actions 压缩包应来自同一提交。

## 代码位置

| 目录 | 内容 |
|---|---|
| `common/`、`tests/` | 原版无线编解码与 ESB 封装、接收盒内部 SPI 消费状态、USB 交付跟踪及兼容测试 |
| `rx-bridge/`、`boards/nologo/` | NCS v2.9.2 接收桥、SPIS2/EasyDMA、GPIOTE/PPI 和实板分区 |
| `gp2040-rx/` | GP2040-CE 应用源码、上游 USB 驱动和网页源码，含 `configs/WirelessRxBox` 和 `src/addons/wireless_rx.cpp` |
| `.github/workflows/build.yml` | 三个 CI 任务、固定依赖和 UF2 校验命令 |

无线队列由 #2 持有。接收 Pico 每次在 READY 高时交换一个 24 字节事务，MISO 为上一拍预装的应答。两块接收板另用 HELLO/OFFER 确认本地启动代次。中立报告带独立标记，Pico 在本机 USB 输入报告成功完成后才从 MOSI 返回对应进度。控制端点、认证包、保活包、短传输和过期完成事件都无法释放无线队首。原板最终快照在本地 SOCD 处理后注入，保留同一报告内的按键和轴变化。XInput 主机的 `01 03 mode` 灯命令沿 SPI、无线 ACK 返回原发射端。

## 来源与许可证

GP2040-CE 基于提交 `0014e4ae2a312332e2582f6708dcc7d6bec5de8c`，其 TinyUSB 分支和 Pico-PIO-USB 分支分别为 `9865cba11ecbcdd25237ba9cf4ccbe3fd1fd821d` 和 `37965f8895fffb4ffacace86e1f731f908dc18e0`。三者的本目标源码和各自许可证已随仓库提供。构建另取 Pico SDK 2.1.1、ArduinoJson 和 NCS，版本见工作流及 west 清单。新增桥接代码采用 MIT。[项目许可证](LICENSE) · [GP2040-CE 许可证](gp2040-rx/LICENSE)
