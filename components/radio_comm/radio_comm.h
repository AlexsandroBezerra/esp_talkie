#define MAC_ADDR_LEN 6

typedef enum {
    MSG_TYPE_PUSH_BUTTON_LEVEL,
} radio_comm_message_type_t;

typedef struct {
    radio_comm_message_type_t type;
    int push_button_level;
} __attribute__((packed)) radio_comm_packet_t;

typedef struct {
    radio_comm_packet_t packet;
    int packet_len;
} radio_comm_event_t;

typedef void (*radio_comm_push_button_level_cb_t)(int level);

void radio_comm_init();

void radio_comm_send_push_button_level(int level);

void radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb_t cb);
