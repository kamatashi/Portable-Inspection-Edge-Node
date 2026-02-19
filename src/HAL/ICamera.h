#pragma once
#include <cstdint>
#include <vector>

struct ImageFrame {
    std::vector<uint8_t> data;
    int width;
    int height;
    bool valid = false;
};

class ICamera {
public:
    virtual ~ICamera() = default;
    virtual bool init() = 0;
    virtual ImageFrame capture() = 0;
    virtual void returnFrame(ImageFrame& frame) = 0; 
};