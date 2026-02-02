idf_component_register(SRCS "main.c" 
                            "camera.c" 
                            "processing.c" 
                            "sensors.c" 
                            "storage.c" 
                            "comms.c"
                       INCLUDE_DIRS "../include"
                       REQUIRES esp_http_client esp_wifi nvs_flash driver esp_timer fatfs esp_driver_sdmmc)