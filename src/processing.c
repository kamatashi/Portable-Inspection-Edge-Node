#include "processing.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG = "PROCESSING";

// Kernels do Sobel (Horizontal e Vertical)
// Gx detecta bordas verticais, Gy detecta horizontais
static const int8_t Gx[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

static const int8_t Gy[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

void process_image(camera_fb_t *fb, scan_data_t *result, int modify_buffer) {
    if (!fb || !result) {
        ESP_LOGE(TAG, "Ponteiro invalido recebido");
        return;
    }

    // Inicializa métricas
    result->total_edge_pixels = 0;
    
    int width = fb->width;
    int height = fb->height;
    uint8_t *img_ptr = fb->buf;

    // Precisamos iterar começando de 1 e parando antes do fim para não acessar
    // memória fora do buffer (bordas da imagem não são processadas)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            
            int sumX = 0;
            int sumY = 0;

            // Aplica a Convolução 3x3
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    // Pega o pixel vizinho
                    // Fórmula: (y+i)*width + (x+j) converte coordenada 2D para índice linear
                    int pixel_val = img_ptr[(y + i) * width + (x + j)];
                    
                    sumX += pixel_val * Gx[i + 1][j + 1];
                    sumY += pixel_val * Gy[i + 1][j + 1];
                }
            }

            // Magnitude do Gradiente
            // O correto seria sqrt(sumX^2 + sumY^2), mas abs(sumX) + abs(sumY) 
            // é muito mais rápido para o processador (Manhattan Distance)
            int magnitude = abs(sumX) + abs(sumY);

            // Binarização (Threshold)
            if (magnitude > EDGE_THRESHOLD) {
                result->total_edge_pixels++;
                
                // Se quisermos visualizar o processamento na imagem enviada
                if (modify_buffer) {
                    img_ptr[y * width + x] = 255; // Pinta de branco
                }
            } else {
                if (modify_buffer) {
                    img_ptr[y * width + x] = 0;   // Pinta de preto
                }
            }
        }
    }

    // Cálculo das Métricas Finais
    int total_pixels = (width - 2) * (height - 2); // Desconta as bordas não processadas
    
    if (total_pixels > 0) {
        result->edge_density = (float)result->total_edge_pixels / total_pixels;
    } else {
        result->edge_density = 0.0f;
    }

    // Cálculo Heurístico de Confiança (Exemplo: densidade muito baixa = parede lisa = confiança alta de não haver fissura?)
    // Ou conforme sua spec: densidade * fator
    float conf_calc = result->edge_density * CONFIDENCE_FACTOR * 100;
    result->confidence = (int)conf_calc;
    if (result->confidence > 100) result->confidence = 100;

    ESP_LOGI(TAG, "Analise concluida: %d pixels de borda, Densidade: %.2f%%", 
             result->total_edge_pixels, result->edge_density * 100);
}