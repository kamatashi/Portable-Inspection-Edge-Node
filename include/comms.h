#ifndef COMMS_H
#define COMMS_H

#include <stdint.h>
#include "esp_err.h"

// Estrutura de dados que vem do processamento (deve alinhar com main.c)
typedef struct {
    char device_id[16];
    float edge_density;
    float estimated_area;
    int confidence;
    // Futuro: ponteiro para buffer da imagem thumbnail
    uint8_t *thumbnail_buf;
    size_t thumbnail_len;
} scan_data_t;

// Inicializa Wi-Fi e UART (Serial)
void comms_init(void);

// Envia dados (Tenta Wi-Fi primeiro, Serial como fallback)
esp_err_t comms_send_data(scan_data_t *data);

#endif