#include "flash_memory.h"
#include "esp_check.h"
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
		ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "failed to erase flash");
		ret = nvs_flash_init();
	}
	ESP_RETURN_ON_ERROR(ret, TAG, "failed to init nvs flash");

	ESP_RETURN_ON_ERROR(nvs_flash_init_partition(PEER_PARTITION), TAG, "failed to init flash partition %s",
						PEER_PARTITION);

	ESP_LOGI(TAG, "flash memory initialized");

	return ESP_OK;
}

esp_err_t flash_memory_get_peer_mac(uint8_t mac[FLASH_MEMORY_MAC_LEN])
{
	if (mac == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	nvs_handle_t handle;
	esp_err_t ret = nvs_open_from_partition(PEER_PARTITION, PEER_NAMESPACE, NVS_READONLY, &handle);
	ESP_RETURN_ON_ERROR(ret, TAG, "open %s/%s failed", PEER_PARTITION, PEER_NAMESPACE);

	uint8_t buf[FLASH_MEMORY_MAC_LEN];
	size_t len = sizeof(buf);
	ret = nvs_get_blob(handle, PEER_MAC_KEY, buf, &len);
	nvs_close(handle);

	ESP_RETURN_ON_ERROR(ret, TAG, "read %s failed", PEER_MAC_KEY);

	if (len != sizeof(buf)) {
		ESP_LOGW(TAG, "%s has %u bytes, expected %u", PEER_MAC_KEY, (unsigned)len, (unsigned)sizeof(buf));
		return ESP_ERR_INVALID_SIZE;
	}

	memcpy(mac, buf, sizeof(buf));
	ESP_LOGI(TAG, "peer mac " MACSTR, MAC2STR(mac));
	return ESP_OK;
}
