#pragma once
#include "../HAL/ICamera.h"
#include <iostream>

class MockCamera : public ICamera {
public:
    bool init() override {
        std::cout << "[MOCK] Camera inicializada (Modo: Simulacao de Fissura).\n";
        return true;
    }

    ImageFrame capture() override {
        std::cout << "[MOCK] Capturando frame 320x240...\n";
        
        ImageFrame frame;
        frame.width = 320;
        frame.height = 240;
        frame.data.resize(320 * 240, 128); // Fundo Cinza (Sem bordas)
        frame.valid = true;

        // --- INJEÇÃO DE FALHA (Fissura Simulada) ---
        // Desenha uma linha preta vertical no meio da imagem
        // Isso garante que o Sobel detecte algo!
        int meio = 160; 
        for(int y = 0; y < 240; y++) {
            // Desenha uma linha de 3 pixels de espessura
            frame.data[y * 320 + meio] = 0;     // Preto
            frame.data[y * 320 + meio + 1] = 0; 
            frame.data[y * 320 + meio + 2] = 0; 
        }

        return frame;
    }

    void returnFrame(ImageFrame& frame) override {
        // Simula a liberação do buffer da câmera
        frame.data.clear();
        frame.valid = false;
    }
};