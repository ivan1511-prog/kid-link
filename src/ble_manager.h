#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>

// ========================================
// BLE Manager
// ========================================
// Модуль управляет BLE: одновременно работает как beacon (передатчик)
// и scanner (приёмник). Это позволяет обоим часам видеть друг друга.
//
// Beacon: периодически рассылает advertisement с нашим UUID пары
// Scanner: слушает эфир и ищет advertisement с нашим UUID

// Инициализация BLE (вызвать в setup())
void ble_init();

// Обновление BLE (вызвать в loop())
// Возвращает true если партнёр найден
void ble_update();

// Получить последний RSSI от партнёра
// Возвращает 0 если партнёр не найден
int ble_get_partner_rssi();

// Получить время последнего обнаружения партнёра (millis)
unsigned long ble_get_last_seen_time();

// Проверить, виден ли партнёр (не потерян сигнал)
bool ble_is_partner_visible();

#endif // BLE_MANAGER_H