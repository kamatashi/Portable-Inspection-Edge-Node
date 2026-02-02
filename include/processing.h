#ifndef PROCESSING_H
#define PROCESSING_H

#include <stdint.h>
#include "esp_camera.h"

// Configurações do Algoritmo
#define EDGE_THRESHOLD  50   // Limiar para considerar um pixel como borda (0-255)
#define CONFIDENCE_FACTOR 2.5 // Fator multiplicador para a heurística de confiança

// Estrutura de dados alinhada com o que o comms.c espera
typedef struct {
    char device_id[16];
    float edge_density;       // 0.0 a 1.0
    int total_edge_pixels;    // Contagem bruta
    int confidence;           // 0 a 100
    uint32_t process_time_ms; // Tempo de execução
} scan_data_t;

/**
 * @brief Processa o frame buffer da câmera usando algoritmo Sobel
 * * @param fb Ponteiro para o frame buffer da câmera (GrayScale)
 * @param result Ponteiro para estrutura onde os resultados serão salvos
 * @param modify_buffer Se 1, desenha as bordas na própria imagem (destrutivo)
 */
void process_image(camera_fb_t *fb, scan_data_t *result, int modify_buffer);

#endif