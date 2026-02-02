#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

// --- MÓDULOS ---
#include "camera.h"
#include "processing.h"
#include "sensors.h"
#include "storage.h"
#include "comms.h"

static const char *TAG = "MAIN";

// --- DEFINIÇÕES DE HARDWARE ---
#define PIN_BUTTON_TRIGGER  12  // Botão de captura (Pull-up)
#define PIN_LED_STATUS      33  // LED Onboard (Lógica invertida: 0=ON)

// --- DEFINIÇÕES DO FREERTOS ---
#define STACK_SIZE_CAMERA   4096
#define STACK_SIZE_PROCESS  8192 // Maior para algoritmo Sobel + Escrita SD
#define STACK_SIZE_COMMS    4096

// --- ESTRUTURAS DE FILA ---

// Estrutura para passar dados da Task 1 (Captura) para Task 2 (Processamento)
typedef struct {
    camera_fb_t *fb;              // Ponteiro para a imagem
    sensor_reading_t sensor_data; // Leitura dos sensores no momento da foto
} processing_request_t;

// Filas
static QueueHandle_t evt_queue = NULL;     // Eventos de botão
static QueueHandle_t proc_queue = NULL;    // Imagem + Sensores -> Processamento
static QueueHandle_t comms_queue = NULL;   // Resultado Final -> Envio

// --- INTERRUPÇÃO (ISR) DO BOTÃO ---
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(evt_queue, &gpio_num, NULL);
}

// ============================================================================
// TAREFA 1: CÂMERA E SENSORES
// Responsabilidade: Capturar o momento (Foto + Dados Ambientais)
// ============================================================================
void camera_sensor_task(void *pvParameters) {
    uint32_t io_num;
    processing_request_t req;

    while(1) {
        // Aguarda botão ser pressionado (Bloqueante)
        if(xQueueReceive(evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "--- INICIANDO CICLO DE INSPECAO ---");
            
            // Feedback Visual (Liga LED)
            gpio_set_level(PIN_LED_STATUS, 0); 

            // 1. Captura Imagem
            req.fb = camera_capture();
            
            if (req.fb) {
                // 2. Lê Sensores IMEDIATAMENTE após a foto (para sincronia temporal)
                req.sensor_data = sensors_read();
                
                ESP_LOGI(TAG, "Captura OK. D:%dmm Luz:%d. Enviando p/ processamento...", 
                         req.sensor_data.distance_mm, req.sensor_data.light_lux);

                // 3. Envia pacote para a fila de processamento
                if (xQueueSend(proc_queue, &req, pdMS_TO_TICKS(500)) != pdTRUE) {
                    ESP_LOGE(TAG, "Fila de processamento cheia! Descartando.");
                    camera_return_fb(req.fb); // Evita memory leak
                }
            } else {
                ESP_LOGE(TAG, "Falha na captura da camera.");
            }
            
            // Feedback Visual (Desliga LED)
            gpio_set_level(PIN_LED_STATUS, 1);
            
            // Debounce simples (evita múltiplos cliques)
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// ============================================================================
// TAREFA 2: PROCESSAMENTO E ARMAZENAMENTO
// Responsabilidade: Algoritmo de Borda + Salvar no SD
// ============================================================================
void processing_task(void *pvParameters) {
    processing_request_t req;
    scan_data_t result;

    while(1) {
        // Aguarda pacote da Task 1
        if(xQueueReceive(proc_queue, &req, portMAX_DELAY)) {
            int64_t start = esp_timer_get_time();
            
            // 1. Prepara metadados básicos
            snprintf(result.device_id, sizeof(result.device_id), "ES-0001");
            result.sensor_data = req.sensor_data; // Copia dados dos sensores recebidos

            // 2. Executa Algoritmo de Borda (Sobel)
            // Nota: Parametro '1' modifica o buffer da imagem para desenhar as bordas
            process_image(req.fb, &result, 1);

            int64_t end_proc = esp_timer_get_time();
            result.process_time_ms = (uint32_t)((end_proc - start) / 1000);

            // 3. Persistência (SD Card)
            // Salvamos AQUI porque ainda temos o ponteiro 'req.fb' válido
            esp_err_t err = storage_save_scan(&result, req.fb);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Falha ao salvar no SD (continuando para envio...)");
            }

            // 4. Libera memória da câmera (CRÍTICO)
            // Se não liberar, o ESP32 trava na próxima foto por falta de RAM
            camera_return_fb(req.fb);

            // 5. Envia apenas o JSON para a fila de comunicação
            // (A imagem pesada já está no SD, não passamos ela pela fila)
            xQueueSend(comms_queue, &result, portMAX_DELAY);
        }
    }
}

// ============================================================================
// TAREFA 3: COMUNICAÇÃO
// Responsabilidade: Tentar enviar via Wi-Fi ou Serial Fallback
// ============================================================================
void comms_task(void *pvParameters) {
    scan_data_t data;

    while(1) {
        // Aguarda resultado do processamento
        if(xQueueReceive(comms_queue, &data, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Preparando envio do Scan (Densidade: %.2f%%)", data.edge_density * 100);

            // Tenta enviar (Wi-Fi -> Serial Fallback)
            comms_send_data(&data);
            
            ESP_LOGI(TAG, "--- CICLO FINALIZADO ---");
        }
    }
}

// ============================================================================
// MAIN APP
// ============================================================================
void app_main(void) {
    // 1. Inicializa NVS (Necessário para Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Configura GPIOs (Botão e LED)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // Trigger ao apertar (assumindo pull-up)
    io_conf.pin_bit_mask = (1ULL << PIN_BUTTON_TRIGGER);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED_STATUS, 1); // LED Off

    // 3. Cria Filas
    evt_queue = xQueueCreate(5, sizeof(uint32_t));
    proc_queue = xQueueCreate(2, sizeof(processing_request_t)); // Fila pesada, keep it small
    comms_queue = xQueueCreate(5, sizeof(scan_data_t));

    // 4. Inicializa Módulos (Hardware)
    // Ordem importa: Câmera e SD são críticos
    if(camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Erro Fatal: Camera");
        return;
    }
    
    storage_init(); // SD Card
    sensors_init(); // I2C / Mock
    comms_init();   // Wi-Fi / Serial

    // 5. Configura Interrupção do Botão
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUTTON_TRIGGER, gpio_isr_handler, (void*) PIN_BUTTON_TRIGGER);

    // 6. Lança as Tarefas
    // Core 1 (App) é melhor para Câmera e Wi-Fi
    // Core 0 (Pro) é melhor para processamento numérico, mas vamos manter simples no Core 1 
    // ou deixar o FreeRTOS decidir (tskNO_AFFINITY) se não houver conflito de WDT.
    
    xTaskCreatePinnedToCore(camera_sensor_task, "CamTask", STACK_SIZE_CAMERA, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(processing_task, "ProcTask", STACK_SIZE_PROCESS, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(comms_task, "CommsTask", STACK_SIZE_COMMS, NULL, 3, NULL, 1);

    ESP_LOGI(TAG, "Sistema 'Portable Edge Node' Iniciado e Aguardando Botao...");
}