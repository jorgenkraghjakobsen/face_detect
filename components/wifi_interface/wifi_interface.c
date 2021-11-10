/*  WIFI related functionality
    Connect to pre defined wifi 
    
    Written by Jørgen Kragh Jakobsen, November 2021, Copenhagen Denmark 
   
*/

#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
//#include "freertos/ringbuf.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_interface.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "ota_server.h"

xTaskHandle t_tcp_client_task;

static const char* TAG = "WIFI";

char mac_address[18];

EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected & ready to make a
 * request */
// static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                   void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
 
  printf("Setting up wifi\n");
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &event_handler, NULL));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,
              .password = WIFI_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
              .pmf_cfg = {.capable = true, .required = false},
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s", WIFI_SSID);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID,
             WIFI_PASSWORD);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  uint8_t base_mac[6];
  // Get MAC address for WiFi station
  esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
  sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", base_mac[0],
          base_mac[1], base_mac[2], base_mac[3], base_mac[4], base_mac[5]);

  // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
  // &event_handler)); ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT,
  // ESP_EVENT_ANY_ID, &event_handler)); vEventGroupDelete(s_wifi_event_group);

  ESP_LOGI(TAG,"Connected");

  // Setup OTA server 
  xTaskCreate(&ota_server_task, "ota_server_task", 4096, NULL, 15, NULL);
}


// Websocket interface 
void tcp_client_task(void *pvParameters);

void wifi_init_socket() { 
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 7, &t_tcp_client_task);
}   
//#define HOST_IP_ADDR CONFIG_SOCKET_ADDRESS
#define PORT 3333

extern uint8_t *wifi_image_buffer; 

SemaphoreHandle_t sem;
int image_size = 0; 
extern uint8_t *meta ; 
int meta_size = 0; 


#define BUFFER_SIZE 640*480
void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = CONFIG_SOCKET_ADDRESS;
    int addr_family = 0;
    int ip_protocol = 0;
    
    ESP_LOGI(TAG,"Allocate memory in SPRAM");
    wifi_image_buffer = (uint8_t *)heap_caps_malloc(
              sizeof(uint8_t) * BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    meta = (uint8_t*)malloc(sizeof(uint8_t) * 8 * 4); 
    
    //Float predition             : 1 x u32_t
    //Bounding box (lr,ll, ur,rl) : 2 x u32_t 
    //Eyes, Nose, Mounth          : 5 x u32_t 

    sem = xSemaphoreCreateMutex();
    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        
        char msg[8] ; 
            
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");
        vTaskDelay(2000 / portTICK_PERIOD_MS); 
        ESP_LOGI(TAG, "Waited 2 sec to start event loop");
        
        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }
            if ((rx_buffer[0] == 1) & (rx_buffer[1] == 1)){    //Return Image from buffer 
                xSemaphoreTake(sem, portMAX_DELAY);
                uint32_t size = image_size;
                
                msg[0] = 1;      // Raw message_type 
                msg[1] = (size & 0x00ff0000)>>16;
                msg[2] = (size & 0x0000ff00)>>8;
                msg[3] = (size & 0x000000ff);
                size = meta_size;  
                msg[4] = 0;      // Meta size  
                msg[5] = (size & 0x00ff0000)>>16;
                msg[6] = (size & 0x0000ff00)>>8;
                msg[7] = (size & 0x000000ff);
                send(sock, msg, 8, 0);
                if (size > 0) {  
                  send(sock, meta, size, 0);
                } 
                err = send(sock, wifi_image_buffer, image_size, 0);
                xSemaphoreGive(sem);
            } else {
                ESP_LOGI(TAG, "Socket command not understood");  
            }
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void wifi_send_image(uint8_t* data, uint size,uint8_t* m, uint m_size ) {
   ESP_LOGI(TAG,"Copy image to local buffer");
   ESP_LOGI(TAG,"Image compact size : %d",size);
   ESP_LOGI(TAG,"Meta size          : %d",m_size);
   
   xSemaphoreTake(sem, portMAX_DELAY);
   memcpy(wifi_image_buffer,data,size);
   image_size = size;
   meta_size = m_size;
   if (m_size > 0) {
     memcpy(meta,m,m_size); 
   }
   xSemaphoreGive(sem);
   ESP_LOGI(TAG,"Done");
   ESP_LOGI(TAG,"--------------------------------------------------------------------------------------------");
}



