void push_button_init(gpio_num_t gpio_num);

void push_button_set_debounce(int debounce_in_ms);

typedef void (*push_button_level_cb_t)(int level);
void push_button_register_level_cb(push_button_level_cb_t cb);
