#include "button_gpio.h"
#include "esp_log.h"
#include "flash_memory.h"
#include "iot_button.h"
#include "led.h"
#include "radio_comm.h"

static const char *TAG = "main";

static void radio_comm_push_button_level_cb(uint8_t level)
{
	ESP_LOGI(TAG, "Received from radio_comm: %d", level);
	led_set_level(level);
}

static void push_button_press_cb(void *arg, void *data)
{
	uint8_t level = (uint8_t)(uintptr_t)data;

	esp_err_t err = radio_comm_send_push_button_level(level);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "failed to send button level: %s", esp_err_to_name(err));
	}
}

void app_main(void)
{
	led_init(CONFIG_LED_GPIO);

	radio_comm_config_t radio_comm_config = {
		.channel = CONFIG_RADIO_COMM_CHANNEL,
		.magic_number = CONFIG_RADIO_COMM_MAGIC_NUMBER,
		.queue_size = CONFIG_RADIO_COMM_QUEUE_SIZE,
	};

	ESP_ERROR_CHECK(flash_memory_init());
	ESP_ERROR_CHECK(flash_memory_get_peer_mac(radio_comm_config.peer_mac));

	ESP_ERROR_CHECK(radio_comm_init(radio_comm_config));
	radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb);

	const button_config_t btn_config = {0};
	const button_gpio_config_t btn_gpio_config = {
		.gpio_num = CONFIG_PUSH_BUTTON_GPIO,
		.active_level = 0,
		.disable_pull = false,
	};
	button_handle_t btn;
	ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_config, &btn_gpio_config, &btn));
	ESP_ERROR_CHECK(iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, push_button_press_cb, (void *)1));
	ESP_ERROR_CHECK(iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, push_button_press_cb, (void *)0));
}
