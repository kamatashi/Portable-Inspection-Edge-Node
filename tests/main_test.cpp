#include <gtest/gtest.h>
#include "../src/Mocks/MockCamera.h"

// Teste simples para validar se o Mock da câmera funciona
TEST(CameraSystem, CaptureReturnsValidFrame) {
    MockCamera cam;
    ASSERT_TRUE(cam.init()); // Verifica se inicia

    ImageFrame frame = cam.capture();
    
    // Verifica requisitos básicos da imagem
    EXPECT_EQ(frame.width, 320);
    EXPECT_EQ(frame.height, 240);
    EXPECT_FALSE(frame.data.empty());
}

// Futuro teste do Processamento de Borda
TEST(EdgeProcessing, DetectsFailures) {
    // Aqui você vai instanciar sua classe 'EdgeProcessor'
    // E passar a imagem do MockCamera para ela
}