# AgentDock

AI 开发者桌面伴侣：带 USB Hub 的小屏幕，显示 AI Agent 任务进度。

## 硬件

- ESP32-S3 + GL850G USB Hub
- ST7789 240×240 2.0" 屏幕
- USB-C 一线连 PC，分出 3×USB-A
- 全 5V 低压供电，2 层板

## 软件

- **PC 端**：Python 采集 Agent 状态 → USB CDC 串口发 JSON
- **固件**：ESP-IDF + LVGL，USB CDC 收数据 → 刷新屏幕
- **扩展**：WiFi 时钟/天气 + BLE 手机通知

## 目录

```
agent-dock/
├── firmware/      # ESP32-S3 固件
├── pc-app/        # PC 端 Python
├── hardware/      # 原理图 + PCB
├── docs/          # 设计文档
└── README.md
```

## 许可证

硬件设计 & 软件代码：[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)
