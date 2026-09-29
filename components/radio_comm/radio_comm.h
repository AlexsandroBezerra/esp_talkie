#define MAC_ADDR_LEN 6

typedef enum {
    MSG_TYPE_PPT_STATUS,
} radio_comm_message_type_t;

typedef struct {
    radio_comm_message_type_t type;
    int ppt_status;
} __attribute__((packed)) radio_comm_packet_t;

typedef struct {
    radio_comm_packet_t packet;
    int packet_len;
} radio_comm_event_t;

typedef void (*radio_comm_ppt_status_cb_t)(int status);

void radio_comm_init();

void radio_comm_send_ppt_status(int status);

void radio_comm_register_ppt_status_callback(radio_comm_ppt_status_cb_t cb);
