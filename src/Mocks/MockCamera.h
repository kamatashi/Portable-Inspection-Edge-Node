#pragma once
#include "ICamera.h"
#include <iostream>

class MockCamera : public ICamera {
public:
    bool init() override {
        std::cout << "[MOCK] Camera inicializada.\n";
        return true;
    }

    ImageFrame capture() override {
        std::cout << "[MOCK] Capturando imagem simulada (Ruido cinza)...\n";
        // Cria uma imagem 320x240 cinza para teste
        ImageFrame frame;
        frame.width = 320;
        frame.height = 240;
        frame.data.resize(320 * 240, 128); // Preenche com cinza (valor 128)
        frame.valid = true;
        return frame;
    }

    void returnFrame(ImageFrame& frame) override {
        frame.data.clear();
    }
};