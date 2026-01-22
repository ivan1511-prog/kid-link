#ifndef UI_H
#define UI_H

#include <Arduino.h>

// ========================================
// UI модуль - управление дисплеем
// ========================================
// T-Watch-S3 имеет дисплей ST7789 240x240.
// Используем TFT_eSPI для отрисовки.

// Состояния для отображения
enum UIState {
    UI_STATE_OK,        // Зелёный - всё хорошо
    UI_STATE_FAR,       // Жёлтый - предупреждение
    UI_STATE_NO_SIGNAL  // Красный - потеря сигнала
};

// Инициализация дисплея
void ui_init();

// Обновить экран с текущим состоянием
void ui_update(UIState state, int rssi);

// Показать экран поиска
void ui_show_searching();

#endif // UI_H
