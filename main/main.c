#include "esp_log.h"
#include "led.h"
#include "radio_comm.h"
#include "push_button.h"

static const char* TAG = "main";

static void radio_comm_ppt_status_cb(int status)
{
    ESP_LOGI(TAG, "Received: %d", status);
    led_set_level(status);
}

static void push_button_cb(int level)
{
    ESP_LOGI(TAG, "Push button level: %d", level);
    radio_comm_send_ppt_status(level);
}

void app_main(void)
{
    led_init(CONFIG_LED_GPIO);

    radio_comm_init();
    radio_comm_register_ppt_status_cb(radio_comm_ppt_status_cb);

    push_button_init(CONFIG_PPT_GPIO);
    push_button_register_level_cb(push_button_cb);
}
