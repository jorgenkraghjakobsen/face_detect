idf_component_register( SRCS "wifi_interface.c"
                        INCLUDE_DIRS "include"
                        REQUIRES ota_server
                        PRIV_REQUIRES nvs_flash ota_server 
                      )
