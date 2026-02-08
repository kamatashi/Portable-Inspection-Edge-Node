#include "EdgeProcessor.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

// Helper para acesso seguro ao vetor de pixels (evita crash nas bordas)
inline uint8_t getPixel(const std::vector<uint8_t>& data, int w, int x, int y) {
    if (x < 0) x = 0;
    if (x >= w) x = w - 1;
    // Assumindo imagem linearizada
    return data[y * w + x];
}

AnalysisResult EdgeProcessor::analyze(const ImageFrame& frame) {
    int w = frame.width;
    int h = frame.height;
    
    // 1. Pipeline: Grayscale (Assumindo que a entrada já é grayscale ou canal Y)
    // Se fosse RGB, faríamos a conversão aqui. O ESP32-CAM costuma entregar YUV/Grayscale.
    std::vector<uint8_t> grayImage = frame.data; 

    // 2. Pipeline: Blur Gaussiano 3x3 
    // Reduz ruído antes de detectar bordas
    std::vector<uint8_t> blurredImage(w * h);
    // Kernel Gaussiano simples (aproximação)
    // 1 2 1
    // 2 4 2
    // 1 2 1
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int sum = 0;
            sum += getPixel(grayImage, w, x-1, y-1) * 1;
            sum += getPixel(grayImage, w, x  , y-1) * 2;
            sum += getPixel(grayImage, w, x+1, y-1) * 1;
            sum += getPixel(grayImage, w, x-1, y  ) * 2;
            sum += getPixel(grayImage, w, x  , y  ) * 4;
            sum += getPixel(grayImage, w, x+1, y  ) * 2;
            sum += getPixel(grayImage, w, x-1, y+1) * 1;
            sum += getPixel(grayImage, w, x  , y+1) * 2;
            sum += getPixel(grayImage, w, x+1, y+1) * 1;
            blurredImage[y * w + x] = sum / 16;
        }
    }

    // 3. Pipeline: Sobel (Detecção de Bordas) 
    std::vector<uint8_t> sobelImage(w * h, 0);
    int edgePixelCount = 0;
    int threshold = 100; // Sensibilidade da detecção [cite: 71]

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            // Gradiente X
            int gx = -1 * getPixel(blurredImage, w, x-1, y-1) + 1 * getPixel(blurredImage, w, x+1, y-1)
                     -2 * getPixel(blurredImage, w, x-1, y  ) + 2 * getPixel(blurredImage, w, x+1, y  )
                     -1 * getPixel(blurredImage, w, x-1, y+1) + 1 * getPixel(blurredImage, w, x+1, y+1);

            // Gradiente Y
            int gy = -1 * getPixel(blurredImage, w, x-1, y-1) - 2 * getPixel(blurredImage, w, x  , y-1) - 1 * getPixel(blurredImage, w, x+1, y-1)
                     +1 * getPixel(blurredImage, w, x-1, y+1) + 2 * getPixel(blurredImage, w, x  , y+1) + 1 * getPixel(blurredImage, w, x+1, y+1);

            // Magnitude
            int magnitude = std::sqrt(gx*gx + gy*gy);
            
            // Thresholding e Contagem
            uint8_t pixelVal = (magnitude > 255) ? 255 : magnitude;
            sobelImage[y * w + x] = pixelVal;

            if (pixelVal > threshold) {
                edgePixelCount++;
            }
        }
    }

    // 4. Extração de Métricas [cite: 31, 75]
    float density = (float)edgePixelCount / (w * h);
    
    // Imprimir "Visualização ASCII" para o log do relatório (Truque para testar sem monitor)
    std::cout << "\n[ASCII ART PREVIEW - SOBEL OUTPUT]\n";
    for(int y = 0; y < h; y += 10) { // Pula linhas para caber na tela
        for(int x = 0; x < w; x += 5) {
            std::cout << (sobelImage[y*w+x] > threshold ? "#" : ".");
        }
        std::cout << "\n";
    }
    std::cout << "[FIM PREVIEW]\n";

    return { density, 0.95f, 0 }; // O tempo deve ser medido por quem chama
}

// (Remova a função applySobel antiga do .h se ela existir separada, integramos tudo acima)