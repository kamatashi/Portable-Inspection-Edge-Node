#include <gtest/gtest.h>  // <--- ESSA LINHA É OBRIGATÓRIA
#include "EdgeProcessor.h"
#include <iostream>

TEST(EdgeProcessing, DetectsLineCrack) {
    // 1. Criar uma imagem preta (100x100)
    ImageFrame frame;
    frame.width = 100;
    frame.height = 100;
    frame.data.resize(100 * 100, 0); 

    // 2. Desenhar uma linha branca no meio (Simulando uma fissura)
    for(int i = 20; i < 80; i++) {
        frame.data[i * 100 + 50] = 255; 
    }

    // 3. Processar
    EdgeProcessor processor;
    AnalysisResult result = processor.analyze(frame);

    // 4. Validar
    std::cout << "[TESTE] Densidade Detectada: " << result.edge_density << std::endl;
    
    EXPECT_GT(result.edge_density, 0.0); 
    EXPECT_LT(result.edge_density, 0.1); 
}