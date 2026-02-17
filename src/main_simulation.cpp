#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>

// Inclua seus cabeçalhos (ajuste os caminhos se necessário)
#include "HAL/ICamera.h"
#include "Mocks/MockCamera.h"
#include "Core/EdgeProcessor.h"
#include "Core/PacketBuilder.h"
#include "Core/SerialProtocol.h"

int main() {
    std::cout << "=== INICIANDO SISTEMA DE INSPECAO (SIMULACAO) ===\n";

    // 1. Inicialização
    MockCamera camera;
    EdgeProcessor processor;
    
    if (!camera.init()) {
        std::cerr << "Erro ao iniciar camera!\n";
        return -1;
    }

    // Loop Infinito Simulado (3 iterações para teste)
    for (int i = 1; i <= 3; i++) {
        std::cout << "\n>>> CICLO " << i << " <<<\n";

        // A. Captura (Usa o seu Mock)
        ImageFrame frame = camera.capture();

        // B. Processamento (Algoritmo Sobel)
        auto start = std::chrono::high_resolution_clock::now();
        AnalysisResult result = processor.analyze(frame);
        auto end = std::chrono::high_resolution_clock::now();
        
        long time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        result.process_time_ms = time_ms;
        
        std::cout << "    [Processamento] Bordas Detectadas: " << result.edge_density * 100.0f << "%\n";
        std::cout << "    [Processamento] Tempo: " << time_ms << "ms\n";

        // C. Dados de Sensores (Mockados)
        SensorData sensors;
        sensors.imu = {0.1f, 0.0f, 9.8f};
        sensors.distance_mm = 500.0f - (i * 10);
        sensors.light_lux = 300.0f;

        // D. Construção do Pacote JSON
        std::string json = PacketBuilder::build("ESP32-SIM-01", sensors, result);
        
        // E. Serialização com CRC-16 (Protocolo de Contingência)
        std::vector<uint8_t> packet = SerialProtocol::pack(json);

        std::cout << "    [Serial] JSON Gerado: " << json.substr(0, 50) << "... (truncado)\n";
        std::cout << "    [Serial] Enviando " << packet.size() << " bytes via cabo...\n";
        
        // F. Validação Visual do Pacote (Hexdump)
        printf("    [DEBUG HEX] ");
        for(size_t k=0; k<10; k++) printf("%02X ", packet[k]); 
        printf("...\n");

        // G. Limpeza de Memória (Seu Mock usa isso)
        camera.returnFrame(frame); 

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "\n=== SIMULACAO FINALIZADA COM SUCESSO ===\n";
    return 0;
}