idf_component_register(SRCS "radio_comm.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_event esp_netif esp_wifi)
