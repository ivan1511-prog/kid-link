#include "vibro.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// ========================================
// DRV2605L Haptic Driver
// ========================================
// T-Watch-S3 использует DRV2605L для управления вибромотором.
// Это I2C устройство (адрес 0x5A), а НЕ простой GPIO!
// Драйвер поддерживает 123 встроенных эффекта вибрации.
//
// Полезные эффекты (из datasheet раздел 11.2):
// 1  - Strong Click 100%
// 14 - Strong Buzz 100%
// 15 - 750 ms Alert 100%
// 16 - 1000 ms Alert 100%
// 47 - Buzz 1 100%
// 52 - Pulsing Strong 1 100%
// 58 - Transition Click 1 100%
// 64 - Transition Ramp Down Long Smooth 1 100%

// Объект драйвера
static Adafruit_DRV2605 g_drv;

// Флаг инициализации
static bool g_initialized = false;

// Время когда нужно остановить вибрацию (0 = не активна)
static unsigned long g_vibro_off_time = 0;

// ========================================
// Инициализация
// ========================================
void vibro_init() {
    Serial.println("[VIBRO] Initializing DRV2605L...");

    // Инициализируем I2C на правильных пинах T-Watch-S3
    Wire.begin(I2C_SDA, I2C_SCL);

    // Инициализируем DRV2605L
    if (!g_drv.begin()) {
        Serial.println("[VIBRO] ERROR: DRV2605L not found!");
        g_initialized = false;
        return;
    }

    g_initialized = true;

    // Выбираем библиотеку эффектов (1 = для ERM моторов)
    g_drv.selectLibrary(1);

    // Устанавливаем режим внутреннего триггера
    g_drv.setMode(DRV2605_MODE_INTTRIG);

    Serial.println("[VIBRO] DRV2605L initialized OK");
}

// ========================================
// Запуск вибрации определённого типа
// ========================================
void vibro_pulse(VibroType type) {
    if (!g_initialized) {
        Serial.println("[VIBRO] Not initialized!");
        return;
    }

    const char* type_name;

    switch (type) {
        case VIBRO_SOFT:
            // Мягкая вибрация — один короткий импульс
            // Эффект 47 = Buzz 1 (умеренное жужжание)
            g_drv.setWaveform(0, 47);
            g_drv.setWaveform(1, 0);   // Конец
            g_vibro_off_time = millis() + 200;
            type_name = "SOFT (effect 47)";
            break;

        case VIBRO_STRONG:
            // Сильная вибрация — два сильных импульса
            // Эффект 14 = Strong Buzz 100%
            g_drv.setWaveform(0, 14);
            g_drv.setWaveform(1, 14);  // Повтор для усиления
            g_drv.setWaveform(2, 0);   // Конец
            g_vibro_off_time = millis() + 400;
            type_name = "STRONG (effect 14x2)";
            break;

        case VIBRO_ALERT:
            // Тревожная вибрация — серия импульсов
            // Эффект 16 = 1000 ms Alert + дополнительные импульсы
            g_drv.setWaveform(0, 16);  // Длинный алерт
            g_drv.setWaveform(1, 1);   // Strong Click
            g_drv.setWaveform(2, 14);  // Strong Buzz
            g_drv.setWaveform(3, 1);   // Strong Click
            g_drv.setWaveform(4, 0);   // Конец
            g_vibro_off_time = millis() + 1500;
            type_name = "ALERT (effect 16+1+14+1)";
            break;

        default:
            return;
    }

    // Запускаем вибрацию
    g_drv.go();

    Serial.printf("[VIBRO] Pulse %s\n", type_name);
}

// ========================================
// Обновление (вызывать в loop)
// ========================================
void vibro_update() {
    if (!g_initialized) return;

    // Если вибрация активна и время вышло — останавливаем
    if (g_vibro_off_time > 0 && millis() >= g_vibro_off_time) {
        g_drv.stop();
        g_vibro_off_time = 0;
    }
}

// ========================================
// Проверка активности
// ========================================
bool vibro_is_active() {
    return g_vibro_off_time > 0;
}
