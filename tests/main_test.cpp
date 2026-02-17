#include <gtest/gtest.h>  // <--- ESSA LINHA É OBRIGATÓRIA
#include "EdgeProcessor.h"
#include "../src/Core/PacketBuilder.h"
#include <iostream>
#include "../src/Core/SerialProtocol.h"


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


TEST(SystemIntegration, GeneratesValidJSON) {
    // 1. DADOS MOCKADOS (Simulando hardware)
    SensorData fakeSensors;
    fakeSensors.imu = {0.1f, 0.05f, 9.8f, 0.0f, 0.0f, 0.0f}; // Acelerômetro quase parado
    fakeSensors.distance_mm = 450.0f;
    fakeSensors.light_lux = 300.0f;

    AnalysisResult fakeAnalysis;
    fakeAnalysis.edge_density = 0.1234f;
    fakeAnalysis.confidence = 0.85f;
    fakeAnalysis.process_time_ms = 150;

    // 2. EXECUÇÃO
    std::string json = PacketBuilder::build("ESP32-TEST-01", fakeSensors, fakeAnalysis);

    // 3. VALIDAÇÃO (O relatório exige formato correto)
    std::cout << "\n[JSON OUTPUT PREVIEW]\n" << json << "\n------------------\n";

    // Verifica se as chaves obrigatórias estão presentes
    EXPECT_NE(json.find("\"device_id\": \"ESP32-TEST-01\""), std::string::npos);
    EXPECT_NE(json.find("\"distance_mm\": 450"), std::string::npos);
    EXPECT_NE(json.find("\"edge_density\": 0.1234"), std::string::npos);
    EXPECT_NE(json.find("\"algorithm\": \"sobel_v1\""), std::string::npos);
}

TEST(Communication, CreatesValidSerialFrame) {
    // 1. Dados de teste (Um pedaço de JSON)
    std::string jsonPayload = "{\"id\":123,\"status\":\"ok\"}";
    
    // 2. Criar o pacote (Frame)
    std::vector<uint8_t> packet = SerialProtocol::pack(jsonPayload);

    // 3. Validações Básicas
    EXPECT_EQ(packet[0], 0xAA); // Tem de ter o Start Byte
    // Tamanho esperado: 1 (Start) + 2 (Len) + 25 (Payload) + 2 (CRC) = 30 bytes
    EXPECT_EQ(packet.size(), 1 + 2 + jsonPayload.size() + 2);

    // 4. Validar integridade
    EXPECT_TRUE(SerialProtocol::validate(packet));
}

TEST(Communication, DetectsCorruptedData) {
    std::string jsonPayload = "{\"dado_importante\": 1000}";
    std::vector<uint8_t> packet = SerialProtocol::pack(jsonPayload);

    // SIMULAÇÃO DE ERRO: Um bit inverteu no cabo (Ruído)
    // Vamos alterar o byte onde está o número '1'
    packet[10] = '9'; 

    // O validador deve perceber que o CRC não bate mais com o Payload
    bool isValid = SerialProtocol::validate(packet);
    
    EXPECT_FALSE(isValid); // O teste passa se a função retornar FALSE (rejeitar o pacote)
}