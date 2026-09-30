#pragma once

#include "driver/gpio.h"

void led_init(gpio_num_t gpio_num);

void led_set_level(uint32_t level);