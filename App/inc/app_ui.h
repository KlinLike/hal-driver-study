/**
 * @file app_ui.h
 * @brief 按当前模式整屏刷新 OLED
 */
#ifndef APP_UI_H
#define APP_UI_H

/** 清屏并按 APP_MODE_SELECT 调用对应模式的整屏绘制 */
void app_ui_full_redraw(void);

#endif /* APP_UI_H */
