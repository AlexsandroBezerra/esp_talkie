#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

static void IRAM_ATTR ppt_isr_handler(void *args) 
{

    int ppt_state = gpio_get_level(CONFIG_PPT_GPIO);
    gpio_set_level(CONFIG_LED_GPIO, !ppt_state);
    ESP_LOGI(TAG, "LED Level setted to %d", !ppt_state);

}

void app_main(void)
{

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO, 0);
    ESP_LOGI(TAG, "LED GPIO configured");

    gpio_config_t ppt_config = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CONFIG_PPT_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&ppt_config);
    ESP_LOGI(TAG, "PPT GPIO configured");

    gpio_install_isr_service(0);

    gpio_isr_handler_add(CONFIG_PPT_GPIO, ppt_isr_handler, (void *)CONFIG_PPT_GPIO);

}
