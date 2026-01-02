#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_gap_bt_api.h" 

// --- Configuración ---
#define BT_DEVICE_NAME "PowaDCR BT"
// Tamaño del Ring Buffer en bytes. Debe ser suficientemente grande para evitar underruns.
// 8KB es un buen punto de partida.
#define RING_BUFFER_SIZE (8 * 1024)

// --- Variables Globales ---
static RingbufHandle_t audio_ring_buffer = NULL;
static esp_a2d_connection_state_t connection_state = ESP_A2D_CONNECTION_STATE_DISCONNECTED;
static bool bt_audio_enabled = false;
volatile bool bt_connection_changed = false;
volatile esp_a2d_connection_state_t new_connection_state;

// --- Declaración de Callbacks ---
void a2dp_app_connection_state_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
int32_t a2dp_app_audio_data_cb(uint8_t *data, int32_t len);

// --- Funciones Principales ---

// Función que se llama desde ZXProcessor para escribir audio
void write_audio_to_bluetooth(const uint8_t *data, size_t size) {
    if (bt_audio_enabled && audio_ring_buffer != NULL && connection_state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        // Escribimos en el ring buffer sin esperar si está lleno para no bloquear el audio
        xRingbufferSend(audio_ring_buffer, data, size, (TickType_t)0);
    }
}

void startBluetoothAudio() {
    if (bt_audio_enabled) return;
    logln("Starting Bluetooth Audio Emitter...");

    // 1. Crear el Ring Buffer
    audio_ring_buffer = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (audio_ring_buffer == NULL) {
        logln("ERROR: Failed to create Ring Buffer for Bluetooth");
        return;
    }

    // 2. Liberar modo clásico si está en uso
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // 3. Inicializar el controlador BT
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        logln("ERROR: esp_bt_controller_init failed");
        return;
    }
    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
        logln("ERROR: esp_bt_controller_enable failed");
        return;
    }
    if (esp_bluedroid_init() != ESP_OK) {
        logln("ERROR: esp_bluedroid_init failed");
        return;
    }
    if (esp_bluedroid_enable() != ESP_OK) {
        logln("ERROR: esp_bluedroid_enable failed");
        return;
    }

    // 4. Inicializar el perfil A2DP (Audio Source)
    esp_a2d_register_callback(a2dp_app_connection_state_cb);
    esp_a2d_source_register_data_callback(a2dp_app_audio_data_cb);
    esp_a2d_source_init();

    // 5. Establecer el nombre del dispositivo
    esp_bt_dev_set_device_name(BT_DEVICE_NAME);

    // 6. Poner el dispositivo en modo "conectable" y "descubrible"
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    bt_audio_enabled = true;
    logln("Bluetooth Audio Emitter enabled. Ready to connect to a speaker.");
}

void stopBluetoothAudio() {
    if (!bt_audio_enabled) return;
    logln("Stopping Bluetooth Audio Emitter...");

    esp_a2d_source_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    if (audio_ring_buffer != NULL) {
        vRingbufferDelete(audio_ring_buffer);
        audio_ring_buffer = NULL;
    }
    bt_audio_enabled = false;
}

// --- Implementación de Callbacks ---

// Callback para el estado de la conexión
void a2dp_app_connection_state_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    if (event == ESP_A2D_CONNECTION_STATE_EVT) {
        // No hagas NADA pesado aquí. Solo actualiza variables globales.
        new_connection_state = param->conn_stat.state;
        bt_connection_changed = true;
    }
}

// Callback que pide los datos de audio. ¡Esta es la función clave!
int32_t a2dp_app_audio_data_cb(uint8_t *data, int32_t len) {
    if (audio_ring_buffer == NULL) {
        return 0;
    }

    size_t item_size = 0;
    uint8_t *item = (uint8_t *)xRingbufferReceive(audio_ring_buffer, &item_size, (TickType_t)0);

    if (item != NULL) {
        memcpy(data, item, item_size);
        vRingbufferReturnItem(audio_ring_buffer, (void *)item);
        return item_size;
    } else {
        memset(data, 0, len);
        return len;
    }
}