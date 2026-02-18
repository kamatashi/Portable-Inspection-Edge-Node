#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

#include "src/Core/EdgeProcessor.h"
#include "src/Core/PacketBuilder.h"
#include "src/Core/SerialProtocol.h"
#include "src/HAL/EspCamera.h"

const char* SSID = "NOME_DA_SUA_REDE";
const char* PASSWORD = "SENHA_DA_SUA_REDE";
const char* API_ENDPOINT = "http://seu-backend.com/api/ingest-scan";

EspCamera camera;
EdgeProcessor processor;

void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] Iniciando Inspection Edge Node...");

    if (!camera.init()) {
        Serial.println("[ERRO] Falha crítica no hardware da câmara! Reiniciando...");
        delay(5000);
        ESP.restart();
    }
    Serial.println("[HARDWARE] Câmara Inicializada (PSRAM Ativa).");

    WiFi.begin(SSID, PASSWORD);
    Serial.print("[WIFI] Tentando conectar");
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Conectado! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[WIFI] Falha na conexão. Entrando em MODO OFFLINE (Serial/SD).");
    }
}

void loop() {
    Serial.println("\n>>> INICIANDO CICLO DE INSPEÇÃO <<<");

    ImageFrame frame = camera.capture();
    if (!frame.valid) {
        Serial.println("[ERRO] Frame inválido capturado.");
        delay(1000);
        return;
    }

    unsigned long t_inicio = millis();
    AnalysisResult result = processor.analyze(frame);
    unsigned long t_fim = millis();
    result.process_time_ms = (t_fim - t_inicio);

    Serial.printf("[EDGE] Bordas: %.2f%% | Tempo: %lums\n", result.edge_density * 100, result.process_time_ms);

    SensorData sensors;
    sensors.imu = {0.0, 0.0, 9.8};
    sensors.distance_mm = 350.0;
    sensors.light_lux = 400.0;

    String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
    std::string jsonPayload = PacketBuilder::build(chipId.c_str(), sensors, result);
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        
        http.begin(client, API_ENDPOINT);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonPayload.c_str());
        
        if (httpResponseCode > 0) {
            Serial.printf("[CLOUD] Sucesso! Resposta HTTP: %d\n", httpResponseCode);
        } else {
            Serial.printf("[CLOUD] Erro no envio: %s\n", http.errorToString(httpResponseCode).c_str());
            enviarViaSerial(jsonPayload); 
        }
        http.end();
        
    } else {
        Serial.println("[OFFLINE] Wi-Fi indisponível. Usando contingência Serial.");
        enviarViaSerial(jsonPayload);
    }

    camera.returnFrame(frame);

    delay(5000);
}

void enviarViaSerial(const std::string& json) {
    std::vector<uint8_t> pacote = SerialProtocol::pack(json);
    
    Serial.write(pacote.data(), pacote.size());
    Serial.println();
}