/**
 * ST7789 240×240 SPI 显示屏驱动
 */
#pragma once
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

void display_init(void);
esp_lcd_panel_handle_t display_get_panel(void);
