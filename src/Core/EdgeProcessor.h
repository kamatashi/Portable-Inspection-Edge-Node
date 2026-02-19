#pragma once
#include "../HAL/ICamera.h"
#include <vector>
#include <string>

struct AnalysisResult {
    float edge_density;
    float confidence;
    long process_time_ms;
    std::string ascii_map;
};

class EdgeProcessor {
public:
    AnalysisResult analyze(const ImageFrame& frame);
};