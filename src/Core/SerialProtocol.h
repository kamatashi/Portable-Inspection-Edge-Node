#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

class SerialProtocol {
public:
    static constexpr uint8_t START_BYTE = 0xAA; 

    static std::vector<uint8_t> pack(const std::string& payload) {
        std::vector<uint8_t> frame;
        
        frame.push_back(START_BYTE);
        uint16_t length = static_cast<uint16_t>(payload.size());
        
        frame.push_back((length >> 8) & 0xFF); 
        frame.push_back(length & 0xFF);        

        for (char c : payload) {
            frame.push_back(static_cast<uint8_t>(c));
        }

        uint16_t crc = calculateCRC16(payload); 
        frame.push_back((crc >> 8) & 0xFF);
        frame.push_back(crc & 0xFF);

        return frame;
    }

    
    static bool validate(const std::vector<uint8_t>& frame) {
        if (frame.size() < 5) return false;
        if (frame[0] != START_BYTE) return false;

        uint16_t len = (frame[1] << 8) | frame[2];
        if (frame.size() != (size_t)(len + 5)) return false;

        std::string payload(frame.begin() + 3, frame.end() - 2);
        
        uint16_t calculated_crc = calculateCRC16(payload);
        uint16_t received_crc = (frame[frame.size()-2] << 8) | frame[frame.size()-1];

        return calculated_crc == received_crc;
    }

private:
    static uint16_t calculateCRC16(const std::string& data) {
        uint16_t crc = 0xFFFF;
        for (char c : data) {
            crc ^= (static_cast<uint8_t>(c) << 8);
            for (int i = 0; i < 8; i++) {
                if (crc & 0x8000)
                    crc = (crc << 1) ^ 0x1021;
                else
                    crc <<= 1;
            }
        }
        return crc;
    }
};