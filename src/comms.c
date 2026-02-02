#include "comms.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/uart.h"

static const char *TAG = "COMMS";

// --- CONFIGURAÇÕES (Mover para Kconfig/NVS depois) ---
#define ESP_WIFI_SSID      "SEU_WIFI"
#define ESP_WIFI_PASS      "SUA_SENHA"
#define BACKEND_URL        "http://seuserver.com/api/engscan/ingest-scan"
#define DEVICE_ID          "ES-0001"

// --- CONTROLE DE WI-FI ---
#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect(); // Tenta reconectar infinito (ajustar backoff depois)
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Tentando reconectar ao Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Conectado ao Wi-Fi!");
    }
}

void comms_wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// --- PROTOCOLO SERIAL (FALLBACK) ---
// Requisito: Framing byte-oriented com CRC-16 [cite: 98]
// Frame: [START(0xAA)] [LEN_H] [LEN_L] [PAYLOAD...] [CRC_H] [CRC_L]

uint16_t calculate_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021; // Polinômio CCITT
            else crc <<= 1;
        }
    }
    return crc;
}

void comms_send_serial_fallback(const char *json_payload) {
    uint16_t len = strlen(json_payload);
    uint8_t header[3] = { 0xAA, (len >> 8) & 0xFF, len & 0xFF };
    uint16_t crc = calculate_crc16((uint8_t*)json_payload, len);
    uint8_t footer[2] = { (crc >> 8) & 0xFF, crc & 0xFF };

    uart_write_bytes(UART_NUM_0, (const char*)header, 3);
    uart_write_bytes(UART_NUM_0, json_payload, len);
    uart_write_bytes(UART_NUM_0, (const char*)footer, 2);
    
    ESP_LOGW(TAG, "Dados enviados via Serial (Fallback).");
}

// --- HTTP CLIENT ---

esp_err_t comms_send_http(const char *json_payload) {
    esp_http_client_config_t config = {
        .url = BACKEND_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Configura Headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    // esp_http_client_set_header(client, "Authorization", "Bearer SEU_TOKEN"); [cite: 101]

    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 status_code,
                 (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST falhou: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

// --- FUNÇÃO PRINCIPAL DE ENVIO ---

esp_err_t comms_send_data(scan_data_t *data) {
    // 1. Constrói o JSON usando cJSON (Seguro e robusto)
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    // Timestamp pode vir de SNTP, por enquanto hardcoded ou contador
    cJSON_AddStringToObject(root, "timestamp", "2026-02-01T12:00:00Z"); 
    
    // Objeto 'analysis' conforme spec [cite: 91]
    cJSON *analysis = cJSON_CreateObject();
    cJSON_AddNumberToObject(analysis, "edge_density", data->edge_density);
    cJSON_AddNumberToObject(analysis, "confidence", data->confidence);
    cJSON_AddItemToObject(root, "analysis", analysis);

    char *json_string = cJSON_PrintUnformatted(root);
    
    if (json_string == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // 2. Verifica se tem Wi-Fi
    esp_err_t res = ESP_FAIL;
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Enviando via Wi-Fi...");
        // Nota: Implementação simplificada enviando apenas JSON. 
        // Para enviar Imagem + JSON, seria necessário montar um body Multipart.
        res = comms_send_http(json_string);
    } 

    // 3. Fallback ou Falha no Wi-Fi
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi indisponivel ou erro. Usando Serial Fallback.");
        comms_send_serial_fallback(json_string);
        res = ESP_OK; // Consideramos sucesso pois foi para o fallback
    }

    // Limpeza
    free(json_string); // cJSON_Print aloca memória, precisa liberar
    cJSON_Delete(root);
    
    return res;
}

void comms_init(void) {
    // Inicializa UART para debug e fallback
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // Inicializa Wi-Fi
    comms_wifi_init();
}