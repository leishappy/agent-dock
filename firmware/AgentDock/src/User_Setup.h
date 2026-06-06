// ============================================================
// AgentDock 的 TFT_eSPI 配置文件
// ============================================================
// PlatformIO 编译时 src/ 的 include 优先级高于库目录，
// 这个文件会自动取代 TFT_eSPI 自带的 User_Setup.h
// ============================================================

#pragma once

// 阻止 TFT_eSPI 再去加载库自带的 User_Setup.h
#define USER_SETUP_LOADED

#define USER_SETUP_INFO "AgentDock ILI9341 240x320"

// --- 驱动选择 -------------------------------------------------
#define ILI9341_DRIVER

// --- 分辨率 (2.8 寸 ILI9341 = 240×320) -------------------------
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// --- 颜色顺序 -------------------------------------------------
// ST7789 IPS 屏幕常见红蓝交换，两种都试试，哪个颜色对用哪个
//#define TFT_RGB_ORDER TFT_RGB
#define TFT_RGB_ORDER TFT_BGR

// --- 引脚定义 (ESP32-S3) ---------------------------------------
// MOSI = GPIO 11, SCLK = GPIO 12
// CS   = GPIO 10, DC   = GPIO 9
// RST  = GPIO 14, BL   = GPIO 2

#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    14
#define TFT_MOSI   11
#define TFT_SCLK   12
#define TFT_MISO   -1
#define TFT_BL      2

// --- SPI 频率 -------------------------------------------------
#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY    6000000
#define SPI_TOUCH_FREQUENCY   2500000

// --- 字体（必须，否则 setTextFont 无效）-------------------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
