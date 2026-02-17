#pragma once
#include "ICamera.h"
#include "esp_camera.h" // Biblioteca nativa do ESP32
#include <Arduino.h>    // Necessário para Serial.println no ESP32

// Definição dos Pinos para o modelo AI-THINKER (Padrão de mercado)
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

class EspCamera : public ICamera {
public:
    bool init() override {
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
        config.pixel_format = PIXFORMAT_GRAYSCALE; // Já pegamos em cinza para economizar processamento!
        
        // Se tiver PSRAM, usamos alta resolução, senão, baixa.
        if(psramFound()){
            config.frame_size = FRAMESIZE_UXGA; // 1600x1200
            config.jpeg_quality = 10;
            config.fb_count = 2;
        } else {
            config.frame_size = FRAMESIZE_QVGA; // 320x240
            config.jpeg_quality = 12;
            config.fb_count = 1;
        }

        // Inicializa a câmera
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x", err);
            return false;
        }

        // Ajuste fino do sensor para detectar melhor as fissuras
        sensor_t * s = esp_camera_sensor_get();
        s->set_brightness(s, 1);  // Aumenta brilho levemente
        s->set_contrast(s, 1);    // Aumenta contraste (ajuda no Sobel)
        
        return true;
    }

    ImageFrame capture() override {
        ImageFrame frame;
        camera_fb_t * fb = esp_camera_fb_get();
        
        if (!fb) {
            Serial.println("Camera capture failed");
            frame.valid = false;
            return frame;
        }

        // Converte o buffer do ESP32 para a nossa estrutura ImageFrame
        frame.width = fb->width;
        frame.height = fb->height;
        frame.valid = true;
        
        // Aqui está o segredo da performance: 
        // Em vez de copiar pixel por pixel, poderíamos usar ponteiros,
        // mas para garantir segurança, copiamos para o nosso vetor.
        // Como o formato já é GRAYSCALE (definido no init), é byte a byte.
        frame.data.assign(fb->buf, fb->buf + fb->len);
        
        // Devolve o buffer para o driver imediatamente para liberar para a próxima foto
        esp_camera_fb_return(fb); 
        
        return frame;
    }

    void returnFrame(ImageFrame& frame) override {
        // Limpa nosso vetor local
        frame.data.clear();
        frame.valid = false;
    }
};