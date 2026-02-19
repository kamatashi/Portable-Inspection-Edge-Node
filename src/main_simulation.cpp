#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>
#include <string>
#include <filesystem>

#include "HAL/ICamera.h"
#include "Core/EdgeProcessor.h"
#include "Core/PacketBuilder.h"
#include "Core/SerialProtocol.h"
#include "Mocks/FileCamera.h"

namespace fs = std::filesystem; 

void simularServidorCloud(const std::vector<uint8_t>& pacoteRecebido) {
    std::cout << "\n    [SERVIDOR] Recebendo pacote de " << pacoteRecebido.size() << " bytes...\n";
    
    if (SerialProtocol::validate(pacoteRecebido)) {
        std::cout << "    [SERVIDOR] \033[1;32mSUCESSO: CRC Valido!\033[0m\n"; 
        std::string jsonFinal(pacoteRecebido.begin() + 3, pacoteRecebido.end() - 2);
        std::cout << "    [SERVIDOR] JSON: " << jsonFinal.substr(0, 60) << "... (truncado)\n";
    } else {
        std::cout << "    [SERVIDOR] \033[1;31mERRO: Falha no CRC!\033[0m\n"; 
    }
}

void salvarRelatorioVisual(int ciclo, const std::string& mapaVisual) {
    fs::path pastaDestino = "../scans";

    try {
        if (!fs::exists(pastaDestino)) {
            fs::create_directories(pastaDestino);
            std::cout << "    [SISTEMA] Pasta 'scans' criada na raiz.\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "    [ERRO] Nao foi possivel criar a pasta: " << e.what() << "\n";
        return;
    }

    std::string nomeArquivo = "scan_ciclo_" + std::to_string(ciclo) + ".txt";
    fs::path caminhoCompleto = pastaDestino / nomeArquivo; 

    std::ofstream arquivo(caminhoCompleto);
    
    if (arquivo.is_open()) {
        arquivo << "=== RELATORIO DE INSPECAO VISUAL ===\n";
        arquivo << "Ciclo: " << ciclo << "\n";
        arquivo << "Data/Hora: Simulacao em Tempo Real\n";
        arquivo << "Legenda: (.) Liso  (#) Fissura\n";
        arquivo << "-----------------------------------\n";
        arquivo << mapaVisual; 
        arquivo << "-----------------------------------\n";
        arquivo.close();
        std::cout << "    [LOG] Relatorio salvo em: " << caminhoCompleto << "\n";
    } else {
        std::cerr << "    [ERRO] Falha ao escrever no arquivo: " << caminhoCompleto << "\n";
    }
}

int main() {
    std::cout << "==========================================\n";
    std::cout << "   DIGITAL TWIN: VALIDACAO COM FOTO REAL\n";
    std::cout << "==========================================\n";

    FileCamera camera("../teste.jpg"); 
    EdgeProcessor processor;
    
    if (!camera.init()) {
        std::cerr << "[ERRO CRITICO] Imagem '../teste.jpg' nao encontrada.\n";
        std::cerr << "Certifique-se de que a imagem esta na raiz do projeto (fora da pasta src ou build).\n";
        return -1;
    }
    
    std::cout << "[ESP32] Hardware inicializado.\n\n";

    for (int i = 1; i <= 3; i++) {
        std::cout << ">>> CICLO " << i << " <<<\n";

        ImageFrame frame = camera.capture();
        if (!frame.valid) break;

        auto start = std::chrono::high_resolution_clock::now();
        AnalysisResult result = processor.analyze(frame);
        auto end = std::chrono::high_resolution_clock::now();
        
        result.process_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "    [ESP32] Processamento Local:\n";
        std::cout << "        - Dimensoes: " << frame.width << "x" << frame.height << "\n";
        std::cout << "        - Bordas: " << std::fixed << std::setprecision(2) << (result.edge_density * 100.0f) << "%\n";
        
        salvarRelatorioVisual(i, result.ascii_map);

        SensorData sensors = { {0.1f, 0.0f, 9.8f}, (float)(500 - i * 50), 300.0f };
        std::string json = PacketBuilder::build("SIM-CHIP-001", sensors, result);
        std::vector<uint8_t> packet = SerialProtocol::pack(json);

        std::cout << "    [ESP32] Enviando " << packet.size() << " bytes...\n";
        simularServidorCloud(packet);

        camera.returnFrame(frame);
        
        std::cout << "------------------------------------------\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << "[SISTEMA] Simulacao concluida.\n";
    return 0;
}