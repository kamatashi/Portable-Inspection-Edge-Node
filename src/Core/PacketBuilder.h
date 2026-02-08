#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "EdgeProcessor.h" // Para ter acesso ao AnalysisResult

// Estruturas de dados dos sensores (conforme PDF pág. 1 e 4)
struct IMUData {
    float ax, ay, az;
    float gx, gy, gz;
};

struct SensorData {
    IMUData imu;
    float distance_mm;
    float light_lux;
};

class PacketBuilder {
public:
    // Monta o JSON final
    static std::string build(const std::string& deviceId, 
                             const SensorData& sensors, 
                             const AnalysisResult& analysis) {
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"device_id\": \"" << deviceId << "\",\n";
        ss << "  \"timestamp\": \"" << getISOTimestamp() << "\",\n";
        
        // Objeto IMU
        ss << "  \"imu\": {\n";
        ss << "    \"ax\": " << sensors.imu.ax << ", \"ay\": " << sensors.imu.ay << ", \"az\": " << sensors.imu.az << "\n";
        ss << "  },\n";
        
        // Sensores simples
        ss << "  \"distance_mm\": " << sensors.distance_mm << ",\n";
        ss << "  \"light_lux\": " << sensors.light_lux << ",\n";
        
        // Resultado da Análise (Seu Sobel!)
        ss << "  \"analysis\": {\n";
        ss << "    \"edge_density\": " << std::fixed << std::setprecision(4) << analysis.edge_density << ",\n";
        ss << "    \"confidence\": " << analysis.confidence << ",\n";
        ss << "    \"process_time_ms\": " << analysis.process_time_ms << ",\n";
        ss << "    \"algorithm\": \"sobel_v1\"\n";
        ss << "  }\n";
        ss << "}";
        
        return ss.str();
    }

private:
    // Gera data/hora atual no formato ISO8601 (ex: 2026-02-10T14:35:20Z)
    static std::string getISOTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[25];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        return std::string(buf);
    }
};