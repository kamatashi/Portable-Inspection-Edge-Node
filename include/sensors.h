#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

// Estrutura para agrupar todas as leituras
typedef struct {
    // IMU (Acelerômetro) - Unidade: g (gravidade)
    float ax;
    float ay;
    float az;
    
    // Distância (ex: VL53L0X ou Ultrassom) - Unidade: mm
    int distance_mm;
    
    // Luminosidade (ex: LDR ou BH1750) - Unidade: Lux
    int light_lux;
} sensor_reading_t;

/**
 * @brief Inicializa os periféricos dos sensores (I2C, ADC, etc)
 * Se estiver no modo simulação, apenas inicializa a semente aleatória.
 */
void sensors_init(void);

/**
 * @brief Realiza a leitura (ou gera valores simulados)
 * @return Estrutura preenchida com os dados
 */
sensor_reading_t sensors_read(void);

#endif