/*  Camera interface 
    Connect to pre defined wifi 
    
    Written by Jørgen Kragh Jakobsen, November 2021, Copenhagen Denmark 
   
*/
#include <stdio.h>
#include "esp_log.h"
#include "esp_camera.h"
#include "camera_interface.h"


static const char *TAG = "CAMERA";

void camera_init(){
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;       
  config.jpeg_quality = 10;               
  config.fb_count     = 1;                

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return;
  }
  
  /*camera_fb_t *frame_buffer = esp_camera_fb_get();
  ESP_LOGI(TAG,"H      : %d ",frame_buffer->height);
  ESP_LOGI(TAG,"W      : %d ",frame_buffer->width);
  ESP_LOGI(TAG,"Setup buffer resolution ");
  */
  ESP_LOGI(TAG,"Camera ok");
}
