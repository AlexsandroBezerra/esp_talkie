#include "flash_memory.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"

static const char *TAG = "flash_memory";

static const char *PEER_PARTITION = "peer_cfg";
static const char *PEER_NAMESPACE = "peer";
static const char *PEER_MAC_KEY = "mac";

esp_err_t flash_memory_init(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to init nvs flash: %s", esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG, "flash memory initialized");

	ret = nvs_flash_init_partition(PEER_PARTITION);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "failed to init flash partition %s: %s", PEER_PARTITION, esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG, "partition %s initialized", PEER_PARTITION);

	return ESP_OK;
}

esp_err_t flash_memory_get_peer_mac(uint8_t mac[FLASH_MEMORY_MAC_LEN])
{
	if (mac == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	nvs_handle_t handle;
	esp_err_t ret = nvs_open_from_partition(PEER_PARTITION, PEER_NAMESPACE, NVS_READONLY, &handle);
	if (ret != ESP_OK) {
		ESP_LOGW(TAG, "open %s/%s failed: %s", PEER_PARTITION, PEER_NAMESPACE, esp_err_to_name(ret));
		return ret;
	}

	uint8_t buf[FLASH_MEMORY_MAC_LEN];
	size_t len = sizeof(buf);
	ret = nvs_get_blob(handle, PEER_MAC_KEY, buf, &len);
	nvs_close(handle);

	if (ret != ESP_OK) {
		ESP_LOGW(TAG, "read %s failed: %s", PEER_MAC_KEY, esp_err_to_name(ret));
		return ret;
	}
	if (len != sizeof(buf)) {
		ESP_LOGW(TAG, "%s has %u bytes, expected %u", PEER_MAC_KEY, (unsigned)len, (unsigned)sizeof(buf));
		return ESP_ERR_INVALID_SIZE;
	}

	memcpy(mac, buf, sizeof(buf));
	ESP_LOGI(TAG, "peer mac " MACSTR, MAC2STR(mac));
	return ESP_OK;
}
