#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

void app_main(void)
{

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO, 0);
    ESP_LOGI(TAG, "LED GPIO direction setted");

    while (1)
    {
        
        gpio_set_level(CONFIG_LED_GPIO, 1);
        ESP_LOGI(TAG, "LED GPIO setted to HIGH");
        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(CONFIG_LED_GPIO, 0);
        ESP_LOGI(TAG, "LED GPIO setted to LOW");
        vTaskDelay(pdMS_TO_TICKS(1000));

    }

}
