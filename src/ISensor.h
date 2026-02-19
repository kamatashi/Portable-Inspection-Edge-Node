#pragma once

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual float readValue() = 0;
};

struct IMUData {
    float ax, ay, az;
    float gx, gy, gz;
};

class IIMU {
public:
    virtual ~IIMU() = default;
    virtual IMUData read() = 0;
};