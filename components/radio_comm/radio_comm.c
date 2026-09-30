#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "radio_comm.h"

#define RADIO_COMM_CHANNEL 1
#define RADIO_COMM_QUEUE_SIZE 6

static char* TAG = "radio_comm";
static uint8_t radio_comm_broadcast_addr[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static uint16_t radio_comm_max_delay = 512;

static QueueHandle_t s_radio_comm_queue = NULL;
static radio_comm_push_button_level_cb_t radio_comm_push_button_level_cb = NULL;

static void radio_comm_nvs_init();
static void radio_comm_wifi_init();
static void radio_comm_esp_now_init();
static void radio_comm_task(void *pvParameter);
static void radio_comm_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);

void radio_comm_init()
{
    s_radio_comm_queue = xQueueCreate(RADIO_COMM_QUEUE_SIZE, sizeof(radio_comm_event_t));
    if (s_radio_comm_queue == NULL) {
        ESP_LOGI(TAG, "Error when creating radio conn queur");
        abort();
    }

    radio_comm_nvs_init();
    radio_comm_wifi_init();
    radio_comm_esp_now_init();

    xTaskCreate(radio_comm_task, "radio_comm_task", 3072, NULL, 20, NULL);

    ESP_LOGI(TAG, "Radio connection initialized");
}

void radio_comm_send_push_button_level(int status)
{
    radio_comm_packet_t packet;
    packet.type = MSG_TYPE_PUSH_BUTTON_LEVEL;
    packet.push_button_level = status;
    esp_now_send(radio_comm_broadcast_addr, (uint8_t *) &packet, sizeof(packet));
}

void radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb_t cb)
{
    radio_comm_push_button_level_cb = cb;
    ESP_LOGI(TAG, "Push button level callback registered");
}

void radio_comm_set_max_delay(uint8_t max_delay)
{
    radio_comm_max_delay = max_delay;
}

static void radio_comm_nvs_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void radio_comm_wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(RADIO_COMM_CHANNEL, WIFI_SECOND_CHAN_NONE));
}

static void radio_comm_esp_now_init()
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(radio_comm_recv_cb));

    esp_now_peer_info_t *broadcast_peer = malloc(sizeof(esp_now_peer_info_t));
    if (broadcast_peer == NULL) {
        ESP_LOGE(TAG, "Malloc broadcast_peer information fail");
        abort();
    }
    memset(broadcast_peer, 0, sizeof(esp_now_peer_info_t));
    broadcast_peer->channel = RADIO_COMM_CHANNEL;
    broadcast_peer->ifidx = WIFI_IF_STA;
    broadcast_peer->encrypt = false;
    memcpy(broadcast_peer->peer_addr, radio_comm_broadcast_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(broadcast_peer) );
    free(broadcast_peer);
}

static void radio_comm_task(void *pvParameter)
{
    radio_comm_event_t event;

    ESP_LOGI(TAG, "Radio conn task started...");

    while (xQueueReceive(s_radio_comm_queue, &event, portMAX_DELAY) == pdTRUE)
    {
        radio_comm_packet_t packet = event.packet;

        if (packet.type == MSG_TYPE_PUSH_BUTTON_LEVEL) {
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
        ESP_LOGE(TAG, "Receive callback arg error");
        return;
    }

    memcpy(packet, data, len);
    event.packet_len = len;

    if (xQueueSend(s_radio_comm_queue, &event, radio_comm_max_delay) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
    }
}