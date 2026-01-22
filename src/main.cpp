#include <Arduino.h>
#include "config.h"
#include "ble_manager.h"
#include "vibro.h"
#include "ui.h"

// ========================================
// KidLink - Proximity Watch
// ========================================
// Главный файл прошивки. Инициализирует все модули и
// в loop() обновляет состояние и управляет оповещениями.

// ========================================
// Состояния системы
// ========================================
enum State {
    STATE_OK,        // Партнёр близко, всё хорошо
    STATE_FAR,       // Партнёр отдаляется (предупреждение)
    STATE_NO_SIGNAL  // Партнёр потерян
};

// Текущее состояние
static State g_state = STATE_NO_SIGNAL;

// Время последней вибрации
static unsigned long g_last_vibro_time = 0;

// Флаг первого обнаружения партнёра
static bool g_partner_ever_seen = false;

// ========================================
// Интервалы вибрации для разных состояний
// ========================================
#define VIBRO_INTERVAL_FAR       3000  // Каждые 3 сек при удалении
#define VIBRO_INTERVAL_NO_SIGNAL 2000  // Каждые 2 сек при потере сигнала (увеличено для ALERT)

void setup() {
    // Инициализация Serial для отладки
    Serial.begin(115200);
    delay(1000);  // Ждём стабилизации USB-CDC

    Serial.println("================================");
    Serial.println("   KidLink Proximity Watch");
    Serial.println("================================");
    Serial.printf("Pair UUID: %s\n", PAIR_UUID);
    Serial.printf("RSSI OK threshold: %d dBm\n", RSSI_THRESHOLD_OK);
    Serial.printf("RSSI FAR threshold: %d dBm\n", RSSI_THRESHOLD_FAR);
    Serial.println("================================");

    // Инициализация модулей
    ui_init();
    vibro_init();
    ble_init();

    // Тестовая вибрация при старте
    vibro_pulse(VIBRO_SOFT);

    // Показываем экран поиска
    ui_show_searching();

    Serial.println("\nWaiting for partner...\n");
}

void loop() {
    // Обновляем модули
    ble_update();
    vibro_update();

    // Определяем текущее состояние
    State new_state;
    int rssi = 0;

    if (ble_is_partner_visible()) {
        g_partner_ever_seen = true;
        rssi = ble_get_partner_rssi();

        if (rssi > RSSI_THRESHOLD_OK) {
            new_state = STATE_OK;
        } else if (rssi > RSSI_THRESHOLD_FAR) {
            new_state = STATE_FAR;
        } else {
            new_state = STATE_FAR;  // Слишком далеко тоже FAR
        }
    } else {
        new_state = STATE_NO_SIGNAL;
    }

    // Логика вибрации
    unsigned long now = millis();
    unsigned long vibro_interval = 0;

    switch (new_state) {
        case STATE_OK:
            // Не вибрируем
            break;

        case STATE_FAR:
            vibro_interval = VIBRO_INTERVAL_FAR;
            break;

        case STATE_NO_SIGNAL:
            vibro_interval = VIBRO_INTERVAL_NO_SIGNAL;
            break;
    }

    // Вибрируем если нужно и прошло достаточно времени
    if (vibro_interval > 0 && (now - g_last_vibro_time >= vibro_interval)) {
        // Выбираем тип вибрации в зависимости от состояния
        if (new_state == STATE_FAR) {
            vibro_pulse(VIBRO_SOFT);      // Мягкая для предупреждения
        } else if (new_state == STATE_NO_SIGNAL) {
            vibro_pulse(VIBRO_ALERT);     // Тревожная для потери сигнала
        }
        g_last_vibro_time = now;
    }

    // Сбрасываем таймер вибрации при переходе в OK
    if (new_state == STATE_OK && g_state != STATE_OK) {
        g_last_vibro_time = 0;
    }

    // Обновляем UI
    if (g_partner_ever_seen) {
        UIState ui_state;
        switch (new_state) {
            case STATE_OK:        ui_state = UI_STATE_OK; break;
            case STATE_FAR:       ui_state = UI_STATE_FAR; break;
            case STATE_NO_SIGNAL: ui_state = UI_STATE_NO_SIGNAL; break;
        }
        ui_update(ui_state, rssi);
    }

    // Сохраняем состояние
    g_state = new_state;

    // Выводим статус каждую секунду
    static unsigned long last_print = 0;
    if (now - last_print >= 1000) {
        last_print = now;

        const char* state_str;
        switch (g_state) {
            case STATE_OK:        state_str = "OK"; break;
            case STATE_FAR:       state_str = "FAR"; break;
            case STATE_NO_SIGNAL: state_str = "NO SIGNAL"; break;
        }

        if (ble_is_partner_visible()) {
            Serial.printf("RSSI: %d dBm | State: %s\n", rssi, state_str);
        } else {
            unsigned long last_seen = ble_get_last_seen_time();
            if (last_seen > 0) {
                unsigned long ago = (now - last_seen) / 1000;
                Serial.printf("State: %s | Last seen %lu sec ago\n", state_str, ago);
            } else {
                Serial.printf("State: %s | Searching...\n", state_str);
            }
        }
    }

    delay(10);
}
