#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_camera.h"

#include "image.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "dl_tool.hpp"

#include "ota_server.h"

#define TWO_STAGE 1 /*<! 1: detect by two-stage which is more accurate but slower(with keypoints). */
                    /*<! 0: detect by one-stage which is less accurate but faster(without keypoints). */

static const char* TAG = "MAIN";

extern "C"
{   
    #include "camera_interface.h" 
    #include "wifi_interface.h"
}

uint8_t *wifi_image_buffer = NULL; 
uint8_t *meta = NULL;

extern "C" void app_main(void)
{   
    
    // Init WIFI connect to access point - config using idf.py menuconfig

    wifi_init_sta(); 
    ESP_LOGI(TAG,"WIFI init done");
    
    wifi_init_socket();
    ESP_LOGI(TAG,"WIFI socket connect");
    
    camera_init();
    ESP_LOGI(TAG,"Camera init done");
        
    dl::tool::Latency latency;

    // initialize
#if TWO_STAGE
    HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
    HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
#else // ONE_STAGE
    HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
#endif

    for(;;) { 
        camera_fb_t *fb = esp_camera_fb_get();
        ESP_LOGI(TAG,"H      : %d ",fb->height);
        ESP_LOGI(TAG,"W      : %d ",fb->width);
        ESP_LOGI(TAG,"len    : %d ",fb->len );
        ESP_LOGI(TAG,"Got JPEG convert to RGB888");
        
        void *ptr = NULL;
        uint32_t ARRAY_LENGTH = fb->width * fb->height * 3;  
        ptr = heap_caps_malloc(ARRAY_LENGTH, MALLOC_CAP_SPIRAM); 
        uint8_t *rgb = (uint8_t *)ptr;
        bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);    
        if (!jpeg_converted) ESP_LOGI(TAG,"-Error converting image to RGB- ");
        
        // inference
        latency.start();
    #if TWO_STAGE
        std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)rgb, {(int)fb->height, (int)fb->width, 3});
        std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)rgb, {(int)fb->height, (int)fb->width, 3}, candidates);
        //std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL});
        //std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL}, candidates);
    #else // ONE_STAGE
        std::list<dl::detect::result_t> &results = s1.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL});
    #endif
        latency.end();
        latency.print("Inference latency");
        
        
        int i = 0;
        uint8_t *p = meta;  
        for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
        {   *(p  ) = 0; 
            *(p+1) = 0; 
            *(p+2) = 0;
            *(p+3) = (int8_t)  (prediction->score * 100);
            *(p+4) = (int8_t) ((prediction->box[0] & 0xff00 )>>8);
            *(p+5) = (int8_t)  (prediction->box[0] & 0x00ff );
            *(p+6) = (int8_t) ((prediction->box[1] & 0xff00 )>>8);
            *(p+7) = (int8_t)  (prediction->box[1] & 0x00ff );
            *(p+8) = (int8_t) ((prediction->box[2] & 0xff00 )>>8);
            *(p+9) = (int8_t)  (prediction->box[2] & 0x00ff );
            *(p+10) = (int8_t)((prediction->box[3] & 0xff00 )>>8);
            *(p+11) = (int8_t) (prediction->box[3] & 0x00ff );
            printf("[%d] score: %f, box: [%d, %d, %d, %d]\n", i, prediction->score, prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);
            
    #if TWO_STAGE
            printf("    left eye: (%3d, %3d), ", prediction->keypoint[0], prediction->keypoint[1]);
            printf("right eye: (%3d, %3d)\n", prediction->keypoint[6], prediction->keypoint[7]);
            printf("    nose: (%3d, %3d)\n", prediction->keypoint[4], prediction->keypoint[5]);
            printf("    mouth left: (%3d, %3d), ", prediction->keypoint[2], prediction->keypoint[3]);
            printf("mouth right: (%3d, %3d)\n\n", prediction->keypoint[8], prediction->keypoint[9]);
    #endif
        }
        wifi_send_image(fb->buf,fb->len, meta, 12);
        // Release camera framebuffer
        esp_camera_fb_return(fb);
        heap_caps_free(ptr);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}