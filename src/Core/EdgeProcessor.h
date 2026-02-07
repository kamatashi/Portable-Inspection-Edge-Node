#pragma once
#include "ICamera.h"
#include <vector>
#include <cmath>

struct AnalysisResult {
    float edge_density;
    float confidence;
    int process_time_ms;
};

class EdgeProcessor {
public:
    // Implementa o pipeline: Grayscale -> Blur -> Sobel -> Density
    static AnalysisResult analyze(const ImageFrame& frame);

private:
    // Operador Sobel simplificado para C++ puro
    static float applySobel(const std::vector<uint8_t>& pixels, int w, int h);
};