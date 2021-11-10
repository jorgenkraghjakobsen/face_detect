#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"

//esp_event_loop_handle_t s_wifi_event_group;

/* Hardcoded WiFi configuration that you can set via
   'make menuconfig'.
*/
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define WIFI_MAXIMUM_RETRY CONFIG_WIFI_MAXIMUM_RETRY

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                   void* event_data);

void wifi_init_sta(void);

void wifi_init_socket(void);

void wifi_send_image(uint8_t* data, uint size, uint8_t* meta, uint m_size);