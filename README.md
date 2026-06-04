# BLE 模块化排插 (BLE Modular Power Strip)

一款**物理可热插拔的模块化排插**，通过 BLE 5.0 连接 PC，在排插屏幕和 Web 端实时展示功耗数据与通知信息。

## 系统架构

```
AC 220V → [HLK-PM01] → ESP32-S3 ←SPI→ ST7789 240x240 圆形屏
                          ↓I2C       4x 模块插槽（Pogo Pin 热插拔）
                          ↓UART      HLW8032 电能计量
                          ↓BLE       连接 PC Web 端
```

## 项目结构

```
ble-modular-power-strip/
├── firmware/          # ESP-IDF 固件
│   ├── main/          # 入口 + 任务创建 + 组件依赖
│   └── components/
│       ├── board/     # GPIO 初始化
│       ├── power_meter/  # HLW8032 电能计量（开源驱动集成）
│       └── ble_gatt/  # NimBLE GATT Server
│
├── web-app/           # Web 端
│   └── index.html     # Phase 1 MVP: 单文件仪表盘
│
├── hardware/          # 硬件设计（KiCad）
├── docs/              # 文档
└── tools/             # 辅助工具
```

## 快速开始

### 固件

```bash
cd firmware

# 1. 安装开源 HLW8032 驱动（推荐）
git clone https://github.com/ucukertz/IDF-HLW8032 components/idf-hlw8032
# 然后取消 power_meter.c 中 #define USE_IDF_HLW8032 的注释

# 2. 编译烧录
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

### Web 控制台

直接用 Chrome/Edge 打开 `web-app/index.html`，点击「连接设备」即可。

## 开发阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | MVP: HLW8032 + BLE + 继电器 + Web 控制台 | 🚧 固件完成，待硬件测试 |
| Phase 2 | 显示屏 + 模块检测 + Svelte 迁移 | ⏳ 待开始 |
| Phase 3 | 完整功能 + PCB + 外壳 | ⏳ 待开始 |

## 核心技术栈

- **MCU**: ESP32-S3-WROOM-1 N16R8 (BLE 5.0 + PSRAM 8MB)
- **BLE 栈**: NimBLE (ESP-IDF)
- **电能计量**: HLW8032 (开源驱动: [ucukertz/idf-hlw8032](https://github.com/ucukertz/IDF-HLW8032))
- **Web**: Vanilla JS + Web Bluetooth API (Phase 2 → Svelte 5)
- **显示**: ST7789 + LVGL v9 (Phase 2+)

## BLE 服务 UUID

`d4e8a000-a0b2-4b9a-9c7e-3c5a8b9c0d1e`

详细协议见 `docs/ble-protocol.md`。
