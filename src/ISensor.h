#pragma once

// Interface Genérica para sensores simples (Luz, Distância)
class ISensor {
public:
    virtual ~ISensor() = default;
    virtual float readValue() = 0; // Retorna Lux ou mm
};

struct IMUData {
    float ax, ay, az; // Aceleração
    float gx, gy, gz; // Giroscópio
};

// Interface específica para o IMU
class IIMU {
public:
    virtual ~IIMU() = default;
    virtual IMUData read() = 0;
};