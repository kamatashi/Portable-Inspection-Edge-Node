#include "sensors.h"
#include "esp_log.h"
#include "esp_system.h"
#include <math.h>

static const char *TAG = "SENSORS";

// --- CONFIGURAÇÃO ---
// Defina como 1 para usar dados falsos (Mock), 0 para tentar ler hardware
#define USE_SENSOR_SIMULATION 1 

void sensors_init(void) {
    if (USE_SENSOR_SIMULATION) {
        ESP_LOGI(TAG, "Inicializado em modo SIMULACAO (Mock)");
        // Nenhuma configuração de GPIO necessária aqui
    } else {
        // AQUI entraria a configuração real:
        // i2c_param_config(...)
        // adc1_config_width(...)
        ESP_LOGI(TAG, "Inicializando drivers de hardware I2C/ADC...");
    }
}

// Função auxiliar para gerar float aleatório entre min e max
static float random_float(float min, float max) {
    uint32_t r = esp_random();
    float scale = (float)r / (float)UINT32_MAX;
    return min + scale * (max - min);
}

sensor_reading_t sensors_read(void) {
    sensor_reading_t reading;

    if (USE_SENSOR_SIMULATION) {
        // --- SIMULAÇÃO DE IMU (Acelerômetro) ---
        // Simula o dispositivo parado, levemente inclinado
        // Z deve estar perto de 1.0 (gravidade), com ruído
        reading.ax = random_float(-0.05, 0.05);
        reading.ay = random_float(-0.05, 0.05);
        reading.az = random_float(0.95, 1.05);

        // --- SIMULAÇÃO DE DISTÂNCIA ---
        // Simula inspeção de parede (ex: entre 30cm e 40cm)
        // Adiciona variação para não ficar estático
        reading.distance_mm = (int)random_float(300, 450);

        // --- SIMULAÇÃO DE LUZ ---
        // Ambiente de escritório (300 a 500 lux)
        reading.light_lux = (int)random_float(300, 500);

        ESP_LOGD(TAG, "Simulado - D: %dmm, Luz: %d lux", 
                 reading.distance_mm, reading.light_lux);

    } else {
        // --- LEITURA REAL (Skeleton para o futuro) ---
        // Exemplo de como seria se tivesse I2C/ADC
        
        // mpu6050_get_accel(&reading.ax, &reading.ay, &reading.az);
        // reading.distance_mm = vl53l0x_read_range();
        // reading.light_lux = adc1_get_raw(ADC1_CHANNEL_X);
        
        // Valores padrão caso hardware falhe
        reading.ax = 0; reading.ay = 0; reading.az = 0;
        reading.distance_mm = 0;
        reading.light_lux = 0;
    }

    return reading;
}