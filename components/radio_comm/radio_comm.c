#include "radio_comm.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"

static const char *TAG = "radio_comm";

static uint8_t peer_mac[RADIO_COMM_MAC_LEN];
static uint16_t magic_number;
static QueueHandle_t s_radio_comm_queue = NULL;
static radio_comm_push_button_level_cb_t radio_comm_push_button_level_cb = NULL;

static esp_err_t radio_comm_wifi_init(radio_comm_config_t config);
static esp_err_t radio_comm_esp_now_init(radio_comm_config_t config);
static esp_err_t radio_comm_send(uint8_t type, uint8_t level);
static void radio_comm_task(void *pvParameter);
static void radio_comm_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

esp_err_t radio_comm_init(radio_comm_config_t config)
{
	s_radio_comm_queue = xQueueCreate(config.queue_size, sizeof(radio_comm_event_t));
	if (s_radio_comm_queue == NULL) {
		ESP_LOGI(TAG, "Error when creating radio conn queue");
		return ESP_FAIL;
	}

	magic_number = config.magic_number;
	memcpy(peer_mac, config.peer_mac, RADIO_COMM_MAC_LEN);

	esp_err_t err = radio_comm_wifi_init(config);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to init wifi");

	err = radio_comm_esp_now_init(config);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to init esp_now");

	xTaskCreate(radio_comm_task, "radio_comm_task", 3072, NULL, 20, NULL);

	ESP_LOGI(TAG, "Radio connection initialized");

	return ESP_OK;
}

esp_err_t radio_comm_send_push_button_level(uint8_t level)
{
	return radio_comm_send(MSG_TYPE_PUSH_BUTTON_LEVEL, level);
}

void radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb_t cb)
{
	radio_comm_push_button_level_cb = cb;
	ESP_LOGI(TAG, "Push button level callback registered");
}

static esp_err_t radio_comm_wifi_init(radio_comm_config_t config)
{
	esp_err_t err = esp_netif_init();
	ESP_RETURN_ON_ERROR(err, TAG, "failed to init net");
	err = esp_event_loop_create_default();
	ESP_RETURN_ON_ERROR(err, TAG, "failed to create event loop");
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	err = esp_wifi_init(&cfg);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to init wifi");
	err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to set storage");
	err = esp_wifi_set_mode(WIFI_MODE_STA);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to set wifi mode");
	err = esp_wifi_start();
	ESP_RETURN_ON_ERROR(err, TAG, "failed to start wifi");
	err = esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to set channel");
	return ESP_OK;
}

static esp_err_t radio_comm_esp_now_init(radio_comm_config_t config)
{
	_Static_assert(sizeof(CONFIG_RADIO_COMM_PMK) - 1 == ESP_NOW_KEY_LEN, "PMK must be 16 chars");
	_Static_assert(sizeof(CONFIG_RADIO_COMM_LMK) - 1 == ESP_NOW_KEY_LEN, "LMK must be 16 chars");

	esp_err_t err = esp_now_init();
	ESP_RETURN_ON_ERROR(err, TAG, "failed to init esp_now");

	err = esp_now_register_recv_cb(radio_comm_recv_cb);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to register recv cb");

	err = esp_now_set_pmk((uint8_t *)CONFIG_RADIO_COMM_PMK);
	ESP_RETURN_ON_ERROR(err, TAG, "failed to set pmk");

	esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
	if (peer == NULL) {
		ESP_LOGE(TAG, "Malloc peer information fail");
		abort();
	}
	memset(peer, 0, sizeof(esp_now_peer_info_t));
	peer->channel = config.channel;
	peer->ifidx = WIFI_IF_STA;
	peer->encrypt = true;
	memcpy(peer->lmk, CONFIG_RADIO_COMM_LMK, ESP_NOW_KEY_LEN);
	memcpy(peer->peer_addr, peer_mac, ESP_NOW_ETH_ALEN);

	err = esp_now_add_peer(peer);
	free(peer);
	return err;
}

static esp_err_t radio_comm_send(uint8_t type, uint8_t level)
{
	radio_comm_header_t header;
	header.magic = magic_number;
	header.type = type;

	radio_comm_packet_t packet;
	packet.header = header;
	packet.push_button_level = level;

	return esp_now_send(peer_mac, (uint8_t *)&packet, sizeof(packet));
}

static void radio_comm_task(void *pvParameter)
{
	radio_comm_event_t event;

	ESP_LOGI(TAG, "Radio conn task started...");

	while (xQueueReceive(s_radio_comm_queue, &event, portMAX_DELAY) == pdTRUE) {
		radio_comm_packet_t packet = event.packet;

		if (packet.header.type == MSG_TYPE_PUSH_BUTTON_LEVEL) {
			if (radio_comm_push_button_level_cb == NULL) {
				ESP_LOGW(TAG, "radio_comm_push_button_level_cb is NULL, discarding packet...");
				continue;
			}

			radio_comm_push_button_level_cb(packet.push_button_level);
		}
	}
}

static void radio_comm_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
	radio_comm_event_t event;
	radio_comm_packet_t *packet = &event.packet;

	if (recv_info == NULL || recv_info->src_addr == NULL || data == NULL || len <= 0) {
		ESP_LOGD(TAG, "Receive callback arg error");
		return;
	}

	if (len != sizeof(radio_comm_packet_t)) {
		ESP_LOGD(TAG, "Received a invalid packet, discarding...");
		return;
	}

	if (memcmp(recv_info->src_addr, peer_mac, RADIO_COMM_MAC_LEN) != 0) {
		ESP_LOGD(TAG, "Packet from unknown sender, discarding...");
		return;
	}

	memcpy(packet, data, len);

	if (packet->header.magic != magic_number) {
		ESP_LOGD(TAG, "Invalid magic number, discarding packet...");
		return;
	}

	if (xQueueSend(s_radio_comm_queue, &event, 0) != pdTRUE) {
		ESP_LOGD(TAG, "Queue is full, discarding packet...");
	}
}