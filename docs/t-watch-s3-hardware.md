# T-Watch-S3 Hardware Reference

Документация по железу LilyGO T-Watch-S3 для проекта KidLink.

## Источники информации

- [TTGO_TWatch_Library (ветка t-watch-s3)](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/tree/t-watch-s3)
- [LilyGoLib документация](https://github.com/Xinyuan-LilyGO/LilyGoLib/blob/master/docs/lilygo-t-watch-s3.md)
- [Adafruit DRV2605 Library](https://github.com/adafruit/Adafruit_DRV2605_Library)

## Основные характеристики

| Параметр | Значение |
|----------|----------|
| MCU | ESP32-S3 |
| Flash | 16MB |
| PSRAM | 8MB |
| Дисплей | ST7789 240x240 |
| Частота CPU | до 240 MHz |

## I2C шина

T-Watch-S3 использует одну I2C шину для всех периферийных устройств:

| Параметр | GPIO |
|----------|------|
| SDA | 10 |
| SCL | 11 |

### Устройства на I2C:

| Устройство | Адрес | Назначение |
|------------|-------|------------|
| BMA423 | 0x18 | Акселерометр |
| PCF8563 | 0x51 | RTC (часы реального времени) |
| AXP2101 | 0x34 | PMU (управление питанием) |
| DRV2605L | 0x5A | Haptic driver (вибромотор) |

## Вибромотор (DRV2605L)

**ВАЖНО:** В отличие от T-Watch V1/V3, где вибромотор управлялся напрямую через GPIO 4, в T-Watch-S3 используется **DRV2605L** — I2C haptic driver!

### ⚠️ КРИТИЧЕСКИ ВАЖНО: Питание DRV2605L

**DRV2605L не будет работать без включения питания через AXP2101!**

Питание DRV2605L подаётся через канал **BLDO2** PMU AXP2101. Перед использованием вибромотора необходимо:

```cpp
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

XPowersPMU pmu;
pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
pmu.setBLDO2Voltage(3300);  // 3.3V
pmu.enableBLDO2();
delay(50);  // Дать время на стабилизацию
```

Без этого шага DRV2605L будет отвечать по I2C (begin() вернёт true), но мотор физически не будет вибрировать!

### Особенности DRV2605L:

- I2C адрес: **0x5A** (фиксированный)
- Поддерживает 123 встроенных эффекта вибрации
- T-Watch-S3 использует **ERM** (Eccentric Rotating Mass) мотор
- Библиотека эффектов: **1** (для ERM)
- Библиотека: [Adafruit_DRV2605_Library](https://github.com/adafruit/Adafruit_DRV2605_Library)

### Популярные эффекты:

| ID | Название | Описание |
|----|----------|----------|
| 1 | Strong Click 100% | Сильный щелчок |
| 14 | Strong Buzz 100% | Сильное жужжание |
| 15 | 750ms Alert 100% | Алерт 750мс |
| 16 | 1000ms Alert 100% | Алерт 1 секунда |
| 47 | Buzz 1 100% | Обычное жужжание |
| 52 | Pulsing Strong 1 100% | Пульсирующая сильная |

⚠️ **Эффект 47 очень слабый и может быть незаметен!** Используйте эффект 14 или 1 для ощутимой вибрации.

Полный список: [DRV2605L Datasheet, раздел 11.2](https://www.ti.com/lit/ds/symlink/drv2605l.pdf)

### Полный рабочий пример:

```cpp
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

XPowersPMU pmu;
Adafruit_DRV2605 drv;

void setup() {
    Wire.begin(10, 11);  // SDA=10, SCL=11 для T-Watch-S3

    // ОБЯЗАТЕЛЬНО: включить питание DRV2605 через AXP2101
    pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 10, 11);
    pmu.setBLDO2Voltage(3300);
    pmu.enableBLDO2();
    delay(50);

    // Теперь можно инициализировать DRV2605
    drv.begin();
    drv.selectLibrary(1);  // Библиотека 1 для ERM моторов
    drv.setMode(DRV2605_MODE_INTTRIG);
}

void vibrate() {
    drv.setWaveform(0, 14);  // Эффект Strong Buzz
    drv.setWaveform(1, 0);   // Конец последовательности
    drv.go();
}
```

## Дисплей (ST7789)

| Параметр | GPIO/Значение |
|----------|---------------|
| MOSI | 13 |
| SCLK | 18 |
| CS | 12 |
| DC | 38 |
| RST | -1 (не используется) |
| Backlight | 45 |
| Разрешение | 240x240 |
| SPI порт | HSPI |
| Частота SPI | 40 MHz |

### Важные настройки TFT_eSPI:

```cpp
-DST7789_DRIVER=1
-DINIT_SEQUENCE_3=1
-DCGRAM_OFFSET=1
-DTFT_RGB_ORDER=TFT_RGB    // ВАЖНО! TFT_BGR даёт неправильные цвета
-DTFT_INVERSION_ON=1       // ВАЖНО! Без этого цвета инвертированы
-DUSE_HSPI_PORT=1
```

**ВАЖНО:**
- Без `TFT_INVERSION_ON=1` цвета отображаются инвертированно
- С `TFT_BGR` вместо `TFT_RGB` жёлтый отображается как бирюзовый

## Аудио (MAX98357A) — требует исследования

T-Watch-S3 имеет встроенный динамик с I2S-усилителем **MAX98357A**.

### Что известно:
- Усилитель: MAX98357A (I2S Class-D Mono Amp)
- Интерфейс: I2S (BCLK, WS/LRCLK, DOUT)
- Также есть PDM микрофон

### Что НЕ найдено:
- Конкретные GPIO пины для I2S (BCLK, WS, DOUT)
- Пины не задокументированы на [espboards.dev](https://www.espboards.dev/esp32/lilygo-t-watch-s3/)

### Где искать:
1. [TTGO_TWatch_Library (ветка t-watch-s3)](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/tree/t-watch-s3) — файл `pins_config.h` или схема
2. [ESPHome реализация](https://github.com/velijv/LILYGO-T-Watch-S3-ESPHome)
3. Схема в папке `schematic` репозитория

### Для реализации звука нужно:
1. Найти GPIO пины I2S в официальной документации/схеме
2. Использовать библиотеку ESP32 I2S или [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools)
3. Генерировать простой тон (например, через XT_DAC_Audio или tone generation)

## Другие пины

| Компонент | GPIO |
|-----------|------|
| Боковая кнопка | 0 |
| IR передатчик | 2 |
| PDM микрофон | Да |
| Radio SX1262 | Да |

## Отличия от T-Watch V1/V3

| Функция | V1/V3 | S3 |
|---------|-------|-----|
| Вибромотор | GPIO 4 (прямое управление) | DRV2605L через I2C |
| I2C SDA | 21 | 10 |
| I2C SCL | 22 | 11 |
| Backlight | 12/15 | 45 |

## Power Management (AXP2101)

PMU AXP2101 управляет питанием различных компонентов. I2C адрес: **0x34**.

### Каналы питания:

| Домен | Назначение | Напряжение |
|-------|------------|------------|
| BLDO2 | DRV2605L (вибромотор) | 3.3V |
| BLDO1 | GPS (на S3 Plus) | - |
| DC3 | GPS (старые версии) | - |

### Библиотека XPowersLib

Для управления AXP2101 используется [XPowersLib](https://github.com/lewisxhe/XPowersLib):

```cpp
#define XPOWERS_CHIP_AXP2101  // ОБЯЗАТЕЛЬНО перед #include!
#include <XPowersLib.h>

XPowersPMU pmu;

void setup() {
    Wire.begin(10, 11);

    if (pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 10, 11)) {
        // Включить питание вибромотора
        pmu.setBLDO2Voltage(3300);
        pmu.enableBLDO2();
    }
}
```

### Важные замечания:

- **#define XPOWERS_CHIP_AXP2101** должен быть ПЕРЕД `#include <XPowersLib.h>`
- Без включения BLDO2 вибромотор не работает (даже если DRV2605L отвечает по I2C)
- Рекомендуется добавить `delay(50)` после включения питания для стабилизации
