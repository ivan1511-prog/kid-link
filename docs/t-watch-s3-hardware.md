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

### Особенности DRV2605L:

- I2C адрес: **0x5A** (фиксированный)
- Поддерживает 123 встроенных эффекта вибрации
- Работает с ERM (Eccentric Rotating Mass) моторами
- Библиотека: [Adafruit_DRV2605_Library](https://github.com/adafruit/Adafruit_DRV2605_Library)

### Популярные эффекты:

| ID | Название | Описание |
|----|----------|----------|
| 1 | Strong Click | Сильный щелчок |
| 14 | Strong Buzz | Сильное жужжание |
| 47 | Buzz | Обычное жужжание |
| 52 | Pulsing Strong | Пульсирующая сильная |

Полный список: [DRV2605L Datasheet, раздел 11.2](https://www.ti.com/lit/ds/symlink/drv2605l.pdf)

### Пример кода:

```cpp
#include <Wire.h>
#include <Adafruit_DRV2605.h>

Adafruit_DRV2605 drv;

void setup() {
    Wire.begin(10, 11);  // SDA=10, SCL=11 для T-Watch-S3
    drv.begin();
    drv.selectLibrary(1);  // Библиотека для ERM моторов
    drv.setMode(DRV2605_MODE_INTTRIG);
}

void vibrate() {
    drv.setWaveform(0, 14);  // Эффект Strong Buzz
    drv.setWaveform(1, 0);   // Конец
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
-DTFT_RGB_ORDER=TFT_BGR
-DTFT_INVERSION_ON=1      // ВАЖНО! Без этого цвета инвертированы
-DUSE_HSPI_PORT=1
```

**ВАЖНО:** Без `TFT_INVERSION_ON=1` цвета отображаются неправильно (зелёный → розовый, чёрный → белый).

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

PMU AXP2101 управляет питанием различных доменов:

| Домен | Назначение |
|-------|------------|
| BLDO2 | DRV2605 Enable |
| ... | ... |

Для полного управления питанием рекомендуется использовать библиотеку от LilyGO.
