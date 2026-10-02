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
    radio_comm_config_t radio_comm_config = {
        .channel = CONFIG_RADIO_COMM_CHANNEL,
        .magic_number = CONFIG_RADIO_COMM_MAGIC_NUMBER,
        .queue_size = CONFIG_RADIO_COMM_QUEUE_SIZE,
    };
    
    ESP_ERROR_CHECK(flash_memory_init());
    esp_err_t ret = flash_memory_get_peer_mac(radio_comm_config.peer_mac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "error when getting mac peer: %s", esp_err_to_name(ret));
        return;
    }

    led_init(CONFIG_LED_GPIO);
    
    radio_comm_init(radio_comm_config);
    radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb);

    push_button_init(CONFIG_PUSH_BUTTON_GPIO);
    push_button_register_level_cb(push_button_level_cb);
}
