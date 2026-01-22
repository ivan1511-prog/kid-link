#include "ble_manager.h"
#include "config.h"
#include <NimBLEDevice.h>

// ========================================
// Глобальные переменные модуля
// ========================================

// Последний RSSI от партнёра (0 = не найден)
static int g_partner_rssi = 0;

// Время последнего обнаружения партнёра
static unsigned long g_last_seen_time = 0;

// UUID нашей пары для поиска
static NimBLEUUID g_pair_uuid(PAIR_UUID);

// Указатель на BLE Scan объект
static NimBLEScan* g_scan = nullptr;

// ========================================
// Callback для результатов сканирования
// ========================================
// Этот класс вызывается NimBLE каждый раз когда находит устройство.
// Мы проверяем, содержит ли advertisement наш UUID пары.

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    // Вызывается при обнаружении устройства
    void onResult(NimBLEAdvertisedDevice* device) override {
        // Проверяем, есть ли в advertisement наш Service UUID
        if (device->isAdvertisingService(g_pair_uuid)) {
            // Нашли партнёра! Сохраняем RSSI и время
            g_partner_rssi = device->getRSSI();
            g_last_seen_time = millis();

            Serial.printf("[BLE] Partner found! RSSI: %d dBm\n", g_partner_rssi);
        }
    }
};

// Экземпляр callback-а
static ScanCallbacks g_scan_callbacks;

// ========================================
// Инициализация BLE
// ========================================
void ble_init() {
    Serial.println("[BLE] Initializing...");

    // Настройка фильтра дубликатов ПЕРЕД init()
    // DATA_DEVICE = репортить устройство повторно если изменились данные
    NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);
    NimBLEDevice::setScanDuplicateCacheSize(10);

    // Инициализируем NimBLE с именем устройства
    NimBLEDevice::init("KidLink");

    // ----------------------------------------
    // Настройка Advertising (Beacon)
    // ----------------------------------------
    // Advertising - это когда устройство периодически "кричит" в эфир
    // информацию о себе. Другие устройства могут это услышать.

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();

    // Добавляем наш UUID пары в advertisement
    // Это позволит другим часам найти нас по UUID
    advertising->addServiceUUID(g_pair_uuid);

    // Настраиваем интервал advertising
    // Параметры в единицах 0.625мс, поэтому делим на 0.625
    uint16_t adv_interval = BLE_ADV_INTERVAL_MS / 0.625;
    advertising->setMinInterval(adv_interval);
    advertising->setMaxInterval(adv_interval);

    // Запускаем advertising (будет работать постоянно)
    advertising->start();
    Serial.println("[BLE] Advertising started");

    // ----------------------------------------
    // Настройка Scanner
    // ----------------------------------------
    // Scanner слушает эфир и ловит advertisement от других устройств

    g_scan = NimBLEDevice::getScan();

    // Устанавливаем callback для обработки результатов
    // false = не фильтровать дубликаты (хотим видеть каждый advertisement)
    g_scan->setAdvertisedDeviceCallbacks(&g_scan_callbacks, false);

    // Интервал сканирования - как часто сканер "просыпается"
    // Окно сканирования - как долго сканер слушает эфир
    // Если interval == window, сканер слушает постоянно
    g_scan->setInterval(BLE_SCAN_INTERVAL_MS / 0.625);
    g_scan->setWindow(BLE_SCAN_WINDOW_MS / 0.625);

    // Активное сканирование - запрашивает дополнительные данные
    // Нам не нужно, используем пассивное для экономии энергии
    g_scan->setActiveScan(false);

    // Не сохранять результаты в память (только callback)
    g_scan->setMaxResults(0);

    // Запускаем непрерывное сканирование (0 = бесконечно)
    g_scan->start(0, nullptr, false);
    Serial.println("[BLE] Scanning started");

    Serial.println("[BLE] Init complete!");
}

// ========================================
// Обновление BLE (вызывать в loop)
// ========================================
void ble_update() {
    // Проверяем, не остановилось ли сканирование
    // (может произойти при ошибках)
    if (!g_scan->isScanning()) {
        Serial.println("[BLE] Restarting scan...");
        g_scan->start(0, nullptr, false);
    }
}

// ========================================
// Получить RSSI партнёра
// ========================================
int ble_get_partner_rssi() {
    return g_partner_rssi;
}

// ========================================
// Получить время последнего обнаружения
// ========================================
unsigned long ble_get_last_seen_time() {
    return g_last_seen_time;
}

// ========================================
// Проверить видимость партнёра
// ========================================
bool ble_is_partner_visible() {
    // Партнёр виден если:
    // 1. Мы его вообще видели (last_seen > 0)
    // 2. С момента последнего обнаружения прошло меньше таймаута
    if (g_last_seen_time == 0) {
        return false;
    }

    unsigned long elapsed = millis() - g_last_seen_time;
    return elapsed < SIGNAL_LOST_TIMEOUT_MS;
}