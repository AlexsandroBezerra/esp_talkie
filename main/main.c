#include "esp_log.h"
#include "driver/gpio.h"
#include "radio_comm.h"
#include "push_button.h"

static const char* TAG = "main";

static void radio_comm_ppt_status_cb(int status)
{
    ESP_LOGI(TAG, "Received: %d", status);
    gpio_set_level(CONFIG_LED_GPIO, status);
}

static void push_button_cb(int level)
{
    ESP_LOGI(TAG, "Push button level: %d", level);
    radio_comm_send_ppt_status(level);
}

void app_main(void)
{
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO, 0);
    ESP_LOGI(TAG, "LED GPIO configured");

    radio_comm_init();
    radio_comm_register_ppt_status_cb(radio_comm_ppt_status_cb);

    push_button_init(CONFIG_PPT_GPIO);
    push_button_register_level_cb(push_button_cb);
}
