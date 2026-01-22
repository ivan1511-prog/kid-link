#include "vibro.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// Указываем тип PMU чипа перед включением библиотеки
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

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

// Объект управления питанием AXP2101
static XPowersPMU g_pmu;

// Флаг инициализации
static bool g_initialized = false;

// Время когда нужно остановить вибрацию (0 = не активна)
static unsigned long g_vibro_off_time = 0;

// ========================================
// Инициализация
// ========================================
void vibro_init() {
    Serial.println("[VIBRO] ===== INIT START =====");
    Serial.printf("[VIBRO] I2C pins: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

    // Инициализируем I2C на правильных пинах T-Watch-S3
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("[VIBRO] Wire.begin() done");

    // ========================================
    // Инициализация AXP2101 для включения питания DRV2605
    // ========================================
    Serial.println("[VIBRO] Initializing AXP2101 PMU...");
    if (!g_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        Serial.println("[VIBRO] WARNING: AXP2101 not found!");
    } else {
        Serial.println("[VIBRO] AXP2101 found, enabling BLDO2 for DRV2605...");
        // Включаем BLDO2 (питание DRV2605) на 3.3V
        g_pmu.setBLDO2Voltage(3300);
        g_pmu.enableBLDO2();
        Serial.println("[VIBRO] BLDO2 enabled at 3.3V");
        delay(50);  // Даём время на стабилизацию питания
    }

    // Сканируем I2C шину для проверки
    Serial.println("[VIBRO] Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[VIBRO] Found device at 0x%02X\n", addr);
        }
    }

    // Инициализируем DRV2605L
    Serial.println("[VIBRO] Calling g_drv.begin()...");
    if (!g_drv.begin()) {
        Serial.println("[VIBRO] ERROR: DRV2605L not found!");
        g_initialized = false;
        return;
    }
    Serial.println("[VIBRO] g_drv.begin() OK");

    g_initialized = true;

    // T-Watch-S3 использует ERM мотор, библиотека 1
    g_drv.selectLibrary(1);
    Serial.println("[VIBRO] selectLibrary(1) done - ERM mode");

    // Устанавливаем режим внутреннего триггера
    g_drv.setMode(DRV2605_MODE_INTTRIG);
    Serial.println("[VIBRO] setMode(INTTRIG) done");

    Serial.println("[VIBRO] ===== INIT OK =====");
}

// ========================================
// Запуск вибрации определённого типа
// ========================================
void vibro_pulse(VibroType type) {
    Serial.printf("[VIBRO] vibro_pulse called, type=%d, g_initialized=%d\n", type, g_initialized);

    if (!g_initialized) {
        Serial.println("[VIBRO] Not initialized!");
        return;
    }

    const char* type_name;

    switch (type) {
        case VIBRO_SOFT:
            // Мягкая вибрация — один импульс
            // Эффект 14 = Strong Buzz 100% (ощутимый)
            g_drv.setWaveform(0, 14);
            g_drv.setWaveform(1, 0);   // Конец
            g_vibro_off_time = millis() + 200;
            type_name = "SOFT (effect 14)";
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
    Serial.println("[VIBRO] Calling g_drv.go()...");
    g_drv.go();
    Serial.printf("[VIBRO] go() done, off_time=%lu, Pulse %s\n", g_vibro_off_time, type_name);
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
