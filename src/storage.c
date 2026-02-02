#include "storage.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

static const char *TAG = "STORAGE";

// Define o ponto de montagem (raiz do SD)
#define MOUNT_POINT "/sdcard"

// Mude para 0 para rodar sem cartão SD físico (apenas log no console)
#define USE_REAL_SD_CARD 1 

// Contador de arquivos (em um sistema real, leríamos do disco na inicialização)
static int file_counter = 0;

esp_err_t storage_init(void) {
    if (!USE_REAL_SD_CARD) {
        ESP_LOGW(TAG, "Modo MOCK ativado: SD Card simulado.");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializando SD Card...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Cuidado: true formata seu cartão se der erro!
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    
    // Configuração específica para ESP32-CAM (SDMMC Host)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    
    // IMPORTANTE: Usar modo 1-bit para maior estabilidade e economia de pinos na ESP32-CAM
    host.flags = SDMMC_HOST_FLAG_1BIT; 
    
    // Slot config
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1; // Força 1-bit

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Falha ao montar sistema de arquivos. Cartao inserido?");
        } else {
            ESP_LOGE(TAG, "Falha ao inicializar SD (%s).", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SD Card montado com sucesso.");
    sdmmc_card_print_info(stdout, card);
    
    // Tenta criar diretório de dados se não existir
    struct stat st = {0};
    if (stat(MOUNT_POINT "/data", &st) == -1) {
        mkdir(MOUNT_POINT "/data", 0700);
    }

    return ESP_OK;
}

esp_err_t storage_save_scan(scan_data_t *data, camera_fb_t *fb) {
    file_counter++;
    char filename_base[64];
    
    // Gera nomes de arquivo: /sdcard/data/scan_001.json e .jpg
    snprintf(filename_base, sizeof(filename_base), "%s/data/scan_%03d", MOUNT_POINT, file_counter);

    if (!USE_REAL_SD_CARD) {
        ESP_LOGI(TAG, "[MOCK] Salvando %s.json (Meta: %.2f) e %s.jpg (%u bytes)", 
                 filename_base, data->edge_density, filename_base, fb->len);
        return ESP_OK;
    }

    // --- 1. Salvar JSON ---
    char json_path[70];
    snprintf(json_path, sizeof(json_path), "%s.json", filename_base);
    
    FILE *f_json = fopen(json_path, "w");
    if (f_json == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo JSON para escrita");
        return ESP_FAIL;
    }

    // Escreve o JSON (Replicando estrutura da spec)
    // Nota: Em produção, ideal usar cJSON_Print, mas fprintf economiza RAM aqui
    fprintf(f_json, "{\n");
    fprintf(f_json, "  \"device_id\": \"%s\",\n", data->device_id);
    fprintf(f_json, "  \"scan_id\": %d,\n", file_counter);
    fprintf(f_json, "  \"analysis\": {\n");
    fprintf(f_json, "    \"edge_density\": %.4f,\n", data->edge_density);
    fprintf(f_json, "    \"confidence\": %d,\n", data->confidence);
    fprintf(f_json, "    \"process_time_ms\": %lu\n", data->process_time_ms);
    fprintf(f_json, "  },\n");
    fprintf(f_json, "  \"sensors\": {\n");
    fprintf(f_json, "    \"dist_mm\": %d,\n", data->sensor_data.distance_mm);
    fprintf(f_json, "    \"lux\": %d\n", data->sensor_data.light_lux);
    fprintf(f_json, "  }\n");
    fprintf(f_json, "}\n");
    
    fclose(f_json);

    // --- 2. Salvar Imagem ---
    char img_path[70];
    snprintf(img_path, sizeof(img_path), "%s.jpg", filename_base); // Assumindo JPG
    // Se estiver usando Grayscale RAW, use extensão .raw ou .pgm

    FILE *f_img = fopen(img_path, "wb");
    if (f_img == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir arquivo de imagem");
        return ESP_FAIL;
    }

    size_t written = fwrite(fb->buf, 1, fb->len, f_img);
    fclose(f_img);

    if (written == fb->len) {
        ESP_LOGI(TAG, "Scan salvo: %s (%u bytes)", filename_base, (unsigned int)written);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Erro ao gravar imagem: esperava %u, gravou %u", fb->len, (unsigned int)written);
        return ESP_FAIL;
    }
}

void storage_get_info(uint64_t *total_bytes, uint64_t *used_bytes) {
    if (!USE_REAL_SD_CARD) {
        *total_bytes = 1000000;
        *used_bytes = 500;
        return;
    }

    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;

    /* Get volume information and free clusters of drive 0 */
    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
        /* Get total sectors and free sectors */
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;

        *total_bytes = tot_sect * 512; // Assumindo setor de 512b
        *used_bytes = (*total_bytes) - (fre_sect * 512);
    }
}