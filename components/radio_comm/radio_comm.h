#pragma once

#include <stdint.h>

#define RADIO_COMM_MAC_LEN 6

typedef enum {
    MSG_TYPE_PUSH_BUTTON_LEVEL,
} radio_comm_message_type_t;

typedef struct {
    uint16_t magic;
    uint8_t type;
}__attribute__((packed)) radio_comm_header_t;

typedef struct {
    radio_comm_header_t header;
    uint8_t push_button_level;
} __attribute__((packed)) radio_comm_packet_t;

typedef struct {
    radio_comm_packet_t packet;
    int packet_len;
} radio_comm_event_t;

typedef void (*radio_comm_push_button_level_cb_t)(uint8_t level);

typedef struct {
    uint16_t magic_number;
    uint8_t peer_mac[RADIO_COMM_MAC_LEN];
    uint8_t channel;
    uint8_t queue_size;
} radio_comm_config_t;

void radio_comm_init(radio_comm_config_t config);

void radio_comm_send_push_button_level(uint8_t level);

void radio_comm_register_push_button_level_cb(radio_comm_push_button_level_cb_t cb);
