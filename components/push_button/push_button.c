#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "push_button.h"

static const char* TAG = "push_button";
static gpio_num_t push_button_gpio = GPIO_NUM_NC;
static uint8_t push_button_debounce = 30;

static TaskHandle_t push_button_handle = NULL;
static push_button_level_cb_t push_button_level_cb = NULL;

static void push_button_isr_handler(void *args);
static void push_button_task(void *args);

void push_button_init(gpio_num_t gpio_num)
{
    push_button_gpio = gpio_num;
    gpio_config_t push_button_config = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << push_button_gpio),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&push_button_config);
    ESP_LOGI(TAG, "Push button configured");

    xTaskCreate(push_button_task, "push_button_task", 2048, NULL, 10, &push_button_handle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(push_button_gpio, push_button_isr_handler, (void *)push_button_gpio);
}

void push_button_set_debounce(uint8_t debounce_in_ms)
{
    push_button_debounce = debounce_in_ms;
    ESP_LOGI(TAG, "Push button debounce setted to %d ms", push_button_debounce);
}

void push_button_register_level_cb(push_button_level_cb_t cb)
{
    push_button_level_cb = cb;
    ESP_LOGI(TAG, "Push button callback registered");
}

static void IRAM_ATTR push_button_isr_handler(void *args) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(push_button_handle, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void push_button_task(void *args)
{
    int8_t last_stable_state = -1;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(push_button_debounce));

        int8_t current_state = !gpio_get_level(push_button_gpio);

        if (current_state != last_stable_state)
        {
            last_stable_state = current_state;

            push_button_level_cb(current_state);
        }
    }
}


