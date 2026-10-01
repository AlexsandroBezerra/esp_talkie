#include "esp_log.h"
#include "driver/gpio.h"

static const char* TAG = "led";
static gpio_num_t led_gpio = GPIO_NUM_NC; 

void led_init(gpio_num_t gpio_num)
{
    led_gpio = gpio_num;
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(led_gpio, 0);
    ESP_LOGI(TAG, "LED GPIO configured on GPIO %d", led_gpio);
}

void led_set_level(uint32_t level)
{
    gpio_set_level(led_gpio, level);
}