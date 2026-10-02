#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "radio_comm.h"

static const char* TAG = "radio_comm";

static uint8_t peer_mac[RADIO_COMM_MAC_LEN];
static uint16_t magic_number;
static QueueHandle_t s_radio_comm_queue = NULL;
static radio_comm_push_button_level_cb_t radio_comm_push_button_level_cb = NULL;

static void radio_comm_wifi_init(radio_comm_config_t config);
static void radio_comm_esp_now_init(radio_comm_config_t config);
static void radio_comm_task(void *pvParameter);
static void radio_comm_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void radio_comm_send(uint8_t type, uint8_t level);

void radio_comm_init(radio_comm_config_t config)
{
    s_radio_comm_queue = xQueueCreate(config.queue_size, sizeof(radio_comm_event_t));
    if (s_radio_comm_queue == NULL) {
        ESP_LOGI(TAG, "Error when creating radio conn queur");
        abort();
    }

    magic_number = config.magic_number;
    memcpy(peer_mac, config.peer_mac, RADIO_COMM_MAC_LEN);

    radio_comm_wifi_init(config);
    radio_comm_esp_now_init(config);

    xTaskCreate(radio_comm_task, "radio_comm_task", 3072, NULL, 20, NULL);

    ESP_LOGI(TAG, "Radio connection initialized");
}

void radio_comm_send_push_button_level(uint8_t level)
{
    radio_comm_send(MSG_TYPE_PUSH_BUTTON_LEVEL, level);
}

void radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb_t cb)
{
    radio_comm_push_button_level_cb = cb;
    ESP_LOGI(TAG, "Push button level callback registered");
}

static void radio_comm_send(uint8_t type, uint8_t level)
{
    radio_comm_header_t header;
    header.magic = magic_number;
    header.type = type;

    radio_comm_packet_t packet;
    packet.header = header;
    packet.push_button_level = level;

    esp_now_send(peer_mac, (uint8_t *) &packet, sizeof(packet));
}

static void radio_comm_wifi_init(radio_comm_config_t config)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE));
}

static void radio_comm_esp_now_init(radio_comm_config_t config)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(radio_comm_recv_cb));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        abort();
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = config.channel;
    peer->ifidx = WIFI_IF_STA;
    peer->encrypt = false;
    memcpy(peer->peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);
}

static void radio_comm_task(void *pvParameter)
{
    radio_comm_event_t event;

    ESP_LOGI(TAG, "Radio conn task started...");

    while (xQueueReceive(s_radio_comm_queue, &event, portMAX_DELAY) == pdTRUE)
    {
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

    uint8_t* src_addr = recv_info->src_addr;
    if (src_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGD(TAG, "Receive callback arg error");
        return;
    }

    if (len != sizeof(radio_comm_packet_t)) {
        ESP_LOGD(TAG, "Received a invalid packet, discarding...");
        return;
    }

    memcpy(packet, data, len);
    event.packet_len = len;

    if (packet->header.magic != magic_number) {
        ESP_LOGD(TAG, "Invalid magic number, discarding packet...");
        return;
    }

    if (xQueueSend(s_radio_comm_queue, &event, 0) != pdTRUE) {
        ESP_LOGD(TAG, "Queue is full, discarding packet...");
    }
}