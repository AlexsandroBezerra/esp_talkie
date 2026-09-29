#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "radio_comm.h"

static const char* TAG = "main";
static TaskHandle_t ppt_task_handle = NULL;

static void IRAM_ATTR ppt_isr_handler(void *args) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(ppt_task_handle, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void ppt_task(void *args)
{
    int last_stable_state = -1;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(CONFIG_PPT_DEBOUNCE_MS));

        int current_state = !gpio_get_level(CONFIG_PPT_GPIO);

        if (current_state != last_stable_state)
        {
            last_stable_state = current_state;
            radio_comm_send_ppt_status(current_state);
            ESP_LOGI(TAG, "Sending current ppt state: %d", current_state);
        }
    }
}

static void radio_comm_ppt_status_cb(int status)
{
    ESP_LOGI(TAG, "Received: %d", status);
    gpio_set_level(CONFIG_LED_GPIO, status);
}

void app_main(void)
{
    radio_comm_init();
    radio_comm_register_ppt_status_callback(radio_comm_ppt_status_cb);

    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO, 0);
    ESP_LOGI(TAG, "LED GPIO configured");

    gpio_config_t ppt_config = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CONFIG_PPT_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&ppt_config);
    ESP_LOGI(TAG, "PPT GPIO configured");

    xTaskCreate(ppt_task, "ppt_task", 2048, NULL, 10, &ppt_task_handle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_PPT_GPIO, ppt_isr_handler, (void *)CONFIG_PPT_GPIO);
}
