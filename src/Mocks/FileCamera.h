#pragma once
#include "../HAL/ICamera.h"
#include <iostream>
#include <string>

// Definição necessária para ativar a implementação da biblioteca
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h" // O arquivo que acabamos de baixar

class FileCamera : public ICamera {
    std::string filepath;

public:
    // Construtor que aceita o nome do arquivo
    FileCamera(const std::string& path) : filepath(path) {}

    bool init() override {
        // Verifica se o arquivo existe (tentativa simples)
        FILE* f = fopen(filepath.c_str(), "rb");
        if (f) {
            fclose(f);
            std::cout << "[FILE_CAM] Arquivo '" << filepath << "' encontrado.\n";
            return true;
        }
        std::cerr << "[FILE_CAM] Erro: Arquivo '" << filepath << "' nao encontrado!\n";
        return false;
    }

    ImageFrame capture() override {
        ImageFrame frame;
        int width, height, channels;

        // stbi_load carrega a imagem. 
        // O último parâmetro '1' FORÇA a conversão para Escala de Cinza (Grayscale)
        // Isso é perfeito porque o Sobel só trabalha com cinza.
        unsigned char *img = stbi_load(filepath.c_str(), &width, &height, &channels, 1);

        if (img == NULL) {
            std::cerr << "[FILE_CAM] Falha ao decodificar a imagem.\n";
            frame.valid = false;
        } else {
            frame.width = width;
            frame.height = height;
            frame.valid = true;

            // Copia os dados brutos da biblioteca para o nosso vetor
            // O ESP32 faria algo parecido lendo do buffer DMA
            frame.data.assign(img, img + (width * height));

            // Libera a memória alocada pela biblioteca stb
            stbi_image_free(img);
            
            std::cout << "[FILE_CAM] Imagem carregada: " << width << "x" << height << "px\n";
        }

        return frame;
    }

    void returnFrame(ImageFrame& frame) override {
        frame.data.clear();
        frame.valid = false;
    }
};