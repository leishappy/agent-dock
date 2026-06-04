# 蓝牙模块化排插

## 项目简介

一款**模块化排插**，通过蓝牙（BLE）连接电脑，在排插自带的显示屏上展示信息（如功耗、温度、开关状态、通知等）。

## 核心功能设想

- [ ] **模块化插孔**：每个插孔独立可控，支持热插拔模块
- [ ] **蓝牙连接**：BLE 与 PC/手机通信
- [ ] **显示屏**：排插上集成屏幕，展示：
  - 各插孔实时功耗（电压/电流/功率）
  - 开关状态
  - 温度监控
  - PC 端推送的通知/提醒
- [ ] **电量统计**：按插孔/按时间段统计用电量
- [ ] **远程控制**：通过 PC 端或手机端开关任意插孔

## 技术栈

- **主控芯片**：待定（如 ESP32-S3，支持 BLE + 显示驱动）
- **蓝牙**：BLE 5.0+
- **显示屏**：待定（如 OLED / TFT LCD）
- **电能计量**：待定（如 HLW8032 / BL0937）
- **PC 端**：待定（如 Electron 桌面应用 / Web Bluetooth API）
- **固件**：ESP-IDF / Arduino / PlatformIO

## 目录结构

```
ble-modular-power-strip/
├── firmware/          # 嵌入式固件
├── desktop-app/       # PC 桌面端
├── hardware/          # 原理图、PCB、BOM
├── docs/              # 设计文档
└── README.md
```

## 常用命令

```bash
# 固件编译与烧录
# pio run -e esp32s3
# pio run -t upload

# PC 端
# npm run dev
# npm run build
```
