/**
 * LVGL 界面渲染
 *
 * 进度条、状态图标、任务列表
 */
#pragma once

void ui_init(void);
void ui_update_task_progress(int completed, int total, const char *task_name);
