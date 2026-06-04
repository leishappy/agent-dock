# AgentDock — 嵌入式固件

## 硬件平台

- ESP32-S3-WROOM-1 N16R8
- ST7789 240×240 2.0" SPI 显示屏
- GL850G USB 2.0 Hub

## 组件

- `display/` — ST7789 SPI 驱动
- `usb_cdc/` — USB CDC 虚拟串口通信
- `ui/` — LVGL 界面渲染

## 编译

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```
