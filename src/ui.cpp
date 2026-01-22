#include "ui.h"
#include "config.h"
#include <SPI.h>
#include <TFT_eSPI.h>

// ========================================
// Дисплей ST7789 240x240
// ========================================

// Объект дисплея
static TFT_eSPI g_tft = TFT_eSPI();

// Предыдущее состояние (для оптимизации перерисовки)
static UIState g_prev_state = (UIState)-1;
static int g_prev_rssi = 0;

// Цвета
#define COLOR_OK        TFT_GREEN
#define COLOR_FAR       TFT_YELLOW
#define COLOR_NO_SIGNAL TFT_RED
#define COLOR_BG        TFT_BLACK
#define COLOR_TEXT      TFT_WHITE

// ========================================
// Инициализация
// ========================================
void ui_init() {
    Serial.println("[UI] Initializing display...");

    // Небольшая задержка перед инициализацией
    delay(100);

    g_tft.init();
    Serial.println("[UI] TFT init done");

    g_tft.setRotation(2);  // Поворот 180° (кнопка снизу)
    g_tft.fillScreen(COLOR_BG);
    Serial.println("[UI] Screen cleared");

    // Включаем подсветку
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Заголовок
    g_tft.setTextColor(COLOR_TEXT, COLOR_BG);
    g_tft.setTextDatum(TC_DATUM);  // Top Center
    g_tft.setTextSize(2);
    g_tft.drawString("KidLink", 120, 10);

    Serial.println("[UI] Display initialized");
}

// ========================================
// Отрисовка индикатора состояния
// ========================================
static void draw_status_circle(uint16_t color) {
    // Большой круг в центре экрана
    int cx = 120;
    int cy = 120;
    int radius = 60;

    g_tft.fillCircle(cx, cy, radius, color);
}

// ========================================
// Отрисовка текста состояния
// ========================================
static void draw_status_text(const char* text, uint16_t color) {
    g_tft.setTextColor(color, COLOR_BG);
    g_tft.setTextDatum(TC_DATUM);
    g_tft.setTextSize(2);

    // Очищаем область текста
    g_tft.fillRect(0, 190, 240, 30, COLOR_BG);

    g_tft.drawString(text, 120, 195);
}

// ========================================
// Отрисовка RSSI
// ========================================
static void draw_rssi(int rssi) {
    g_tft.setTextColor(COLOR_TEXT, COLOR_BG);
    g_tft.setTextDatum(TC_DATUM);
    g_tft.setTextSize(3);

    // Очищаем область RSSI в центре круга
    g_tft.fillRect(70, 105, 100, 35, COLOR_BG);

    if (rssi != 0) {
        char buf[16];
        sprintf(buf, "%d", rssi);
        g_tft.drawString(buf, 120, 110);
    }
}

// ========================================
// Обновление экрана
// ========================================
void ui_update(UIState state, int rssi) {
    // Обновляем только если что-то изменилось
    bool state_changed = (state != g_prev_state);
    bool rssi_changed = (rssi != g_prev_rssi);

    if (!state_changed && !rssi_changed) {
        return;
    }

    // Определяем цвет и текст
    uint16_t color;
    const char* text;

    switch (state) {
        case UI_STATE_OK:
            color = COLOR_OK;
            text = "OK";
            break;
        case UI_STATE_FAR:
            color = COLOR_FAR;
            text = "FAR";
            break;
        case UI_STATE_NO_SIGNAL:
            color = COLOR_NO_SIGNAL;
            text = "ALARM";
            break;
        default:
            color = COLOR_BG;
            text = "???";
    }

    // Перерисовываем если состояние изменилось
    if (state_changed) {
        // Очищаем центр экрана (где был текст "Searching...")
        g_tft.fillRect(0, 50, 240, 140, COLOR_BG);

        draw_status_circle(color);
        draw_status_text(text, color);
    }

    // Обновляем RSSI
    if (rssi_changed || state_changed) {
        // Для NO_SIGNAL не показываем RSSI
        if (state == UI_STATE_NO_SIGNAL) {
            draw_rssi(0);
        } else {
            draw_rssi(rssi);
        }
    }

    g_prev_state = state;
    g_prev_rssi = rssi;
}

// ========================================
// Экран поиска
// ========================================
void ui_show_searching() {
    g_tft.fillScreen(COLOR_BG);

    // Заголовок
    g_tft.setTextColor(COLOR_TEXT, COLOR_BG);
    g_tft.setTextDatum(TC_DATUM);
    g_tft.setTextSize(2);
    g_tft.drawString("KidLink", 120, 10);

    // Текст поиска
    g_tft.setTextSize(2);
    g_tft.setTextColor(TFT_CYAN, COLOR_BG);
    g_tft.drawString("Searching...", 120, 110);

    // Сбрасываем предыдущее состояние
    g_prev_state = (UIState)-1;
    g_prev_rssi = 0;
}
