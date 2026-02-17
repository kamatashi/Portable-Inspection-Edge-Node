#pragma once
#include <cstdint>
#include <vector>

// Estrutura simples para representar a imagem na memória
struct ImageFrame {
    std::vector<uint8_t> data;
    int width;
    int height;
    bool valid = false;
};

// Interface da Câmera
class ICamera {
public:
    virtual ~ICamera() = default;
    virtual bool init() = 0;
    virtual ImageFrame capture() = 0;
    // Função para liberar memória (importante no ESP32)
    virtual void returnFrame(ImageFrame& frame) = 0; 
};