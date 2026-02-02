#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"
#include "esp_camera.h"
#include "processing.h" // Para acessar scan_data_t

/**
 * @brief Inicializa o sistema de arquivos (Monta o SD Card)
 * @return ESP_OK se sucesso
 */
esp_err_t storage_init(void);

/**
 * @brief Salva o pacote de inspeção (JSON + Imagem)
 * * @param data Estrutura com os dados processados e sensores
 * @param fb Frame buffer da câmera (imagem)
 * @return ESP_OK se gravou com sucesso
 */
esp_err_t storage_save_scan(scan_data_t *data, camera_fb_t *fb);

/**
 * @brief Retorna o espaço total e usado do SD (para telemetria)
 */
void storage_get_info(uint64_t *total_bytes, uint64_t *used_bytes);

#endif