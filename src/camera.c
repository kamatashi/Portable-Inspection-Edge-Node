#include "camera.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"

// Definição dos pinos para o modelo AI-THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static const char *TAG = "CAMERA";

// Inicializa a câmera
esp_err_t camera_init(void) {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    
    // CONFIGURAÇÃO CRÍTICA PARA SEU PROJETO:
    // PIXFORMAT_GRAYSCALE: Já entrega 1 byte por pixel (0-255).
    // Ideal para o requisito de processamento de borda[cite: 28, 30].
    config.pixel_format = PIXFORMAT_GRAYSCALE; 
    
    // Resolução: QVGA (320x240) é um bom equilíbrio para atingir
    // a meta de processamento < 3s.
    // Se precisar de mais detalhe para o laudo, use VGA (640x480).
    config.frame_size = FRAMESIZE_QVGA;
    
    config.jpeg_quality = 12; // Relevante apenas se mudar para JPEG
    config.fb_count = 1;      // 1 buffer para economizar RAM para o algoritmo

    // Inicializa o driver
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar camera: 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "Camera iniciada com sucesso");
    return ESP_OK;
}

// Captura um frame
camera_fb_t* camera_capture(void) {
    // Adquire o Frame Buffer
    camera_fb_t *fb = esp_camera_fb_get();
    
    if (!fb) {
        ESP_LOGE(TAG, "Falha na captura do Frame Buffer");
        return NULL;
    }
    
    ESP_LOGI(TAG, "Imagem capturada: %u bytes, %dx%d", 
             (unsigned int)fb->len, fb->width, fb->height);
             
    return fb;
}

// Libera a memória do frame (Chamar após o processamento!)
void camera_return_fb(camera_fb_t *fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}