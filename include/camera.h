#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"

// Inicializa o hardware da câmera
esp_err_t camera_init(void);

// Realiza a captura (bloqueante)
camera_fb_t* camera_capture(void);

// Devolve o buffer para o driver
void camera_return_fb(camera_fb_t *fb);

#endif