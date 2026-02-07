#pragma once
#include <string>
#include <vector>

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual bool init() = 0;
    virtual bool saveFile(const std::string& filename, const std::string& content) = 0;
    virtual bool saveBinary(const std::string& filename, const std::vector<uint8_t>& data) = 0;
};