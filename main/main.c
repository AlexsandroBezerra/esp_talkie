#include "esp_log.h"
#include "led.h"
#include "radio_comm.h"
#include "push_button.h"
#include "flash_memory.h"

static const char* TAG = "main";

static void radio_comm_push_button_level_cb(uint8_t level)
{
    ESP_LOGI(TAG, "Received from radio_comm: %d", level);
    led_set_level(level);
}

static void push_button_level_cb(uint8_t level)
{
    ESP_LOGI(TAG, "Push button level: %d", level);
    radio_comm_send_push_button_level(level);
}

void app_main(void)
{
    flash_memory_init();

    led_init(CONFIG_LED_GPIO);

    radio_comm_init();
    radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb);

    push_button_init(CONFIG_PUSH_BUTTON_GPIO);
    push_button_register_level_cb(push_button_level_cb);
}
