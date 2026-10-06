#pragma once

#include "esp_err.h"
#include <stdint.h>

#define FLASH_MEMORY_MAC_LEN 6

esp_err_t flash_memory_init(void);

esp_err_t flash_memory_get_peer_mac(uint8_t mac[FLASH_MEMORY_MAC_LEN]);
