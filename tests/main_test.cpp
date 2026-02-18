#include <gtest/gtest.h>
#include "EdgeProcessor.h"
#include "../src/Core/PacketBuilder.h"
#include <iostream>
#include "../src/Core/SerialProtocol.h"


TEST(EdgeProcessing, DetectsLineCrack) {
    ImageFrame frame;
    frame.width = 100;
    frame.height = 100;
    frame.data.resize(100 * 100, 0); 

    for(int i = 20; i < 80; i++) {
        frame.data[i * 100 + 50] = 255; 
    }

    EdgeProcessor processor;
    AnalysisResult result = processor.analyze(frame);

    std::cout << "[TESTE] Densidade Detectada: " << result.edge_density << std::endl;
    
    EXPECT_GT(result.edge_density, 0.0); 
    EXPECT_LT(result.edge_density, 0.1); 
}


TEST(SystemIntegration, GeneratesValidJSON) {
    SensorData fakeSensors;
    fakeSensors.imu = {0.1f, 0.05f, 9.8f, 0.0f, 0.0f, 0.0f};
    fakeSensors.distance_mm = 450.0f;
    fakeSensors.light_lux = 300.0f;

    AnalysisResult fakeAnalysis;
    fakeAnalysis.edge_density = 0.1234f;
    fakeAnalysis.confidence = 0.85f;
    fakeAnalysis.process_time_ms = 150;

    std::string json = PacketBuilder::build("ESP32-TEST-01", fakeSensors, fakeAnalysis);

    std::cout << "\n[JSON OUTPUT PREVIEW]\n" << json << "\n------------------\n";

    EXPECT_NE(json.find("\"device_id\": \"ESP32-TEST-01\""), std::string::npos);
    EXPECT_NE(json.find("\"distance_mm\": 450"), std::string::npos);
    EXPECT_NE(json.find("\"edge_density\": 0.1234"), std::string::npos);
    EXPECT_NE(json.find("\"algorithm\": \"sobel_v1\""), std::string::npos);
}

TEST(Communication, CreatesValidSerialFrame) {
    std::string jsonPayload = "{\"id\":123,\"status\":\"ok\"}";
    
    std::vector<uint8_t> packet = SerialProtocol::pack(jsonPayload);

    EXPECT_EQ(packet[0], 0xAA);
    EXPECT_EQ(packet.size(), 1 + 2 + jsonPayload.size() + 2);

    EXPECT_TRUE(SerialProtocol::validate(packet));
}

TEST(Communication, DetectsCorruptedData) {
    std::string jsonPayload = "{\"dado_importante\": 1000}";
    std::vector<uint8_t> packet = SerialProtocol::pack(jsonPayload);

    packet[10] = '9'; 

    bool isValid = SerialProtocol::validate(packet);
    
    EXPECT_FALSE(isValid);
}