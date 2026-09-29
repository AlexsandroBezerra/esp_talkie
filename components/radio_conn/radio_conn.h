#define MAC_ADDR_LEN 6

typedef enum {
    MSG_TYPE_PPT_STATUS,
} radio_conn_message_type_t;

typedef struct {
    radio_conn_message_type_t type;
    int ppt_status;
} __attribute__((packed)) radio_conn_packet_t;

typedef struct {
    radio_conn_packet_t packet;
    int packet_len;
} radio_conn_event_t;

typedef void (*radio_conn_ppt_status_cb_t)(int status);

void radio_conn_init();

void radio_conn_send_ppt_status(int status);

void radio_conn_register_ppt_status_callback(radio_conn_ppt_status_cb_t cb);
