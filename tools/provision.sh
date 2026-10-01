#!/bin/bash

set -euo pipefail

if (( $# != 2 )); then
    echo "Error: Exactly 2 arguments required."
    echo "Usage: $0 <port> <csv>"
    exit 1
fi

if [ -z "${IDF_PATH:-}" ]; then
    echo "Error: IDF_PATH not set. Load the ESP-IDF environment first."
    exit 1
fi

# Input vars
ESP_PORT="$1"
CSV_FILE="$2"

CSV_FILE_PATH="$(realpath "$CSV_FILE")"

if [ ! -f "$CSV_FILE_PATH" ]; then
    echo "Error: csv file not found!"
    exit 1
fi

# MAC of the connected board (esptool prints "MAC:   cc:50:e3:b6:ab:bc")
BOARD_MAC="$(esptool --port "$ESP_PORT" read-mac \
    | awk '/^MAC:/{mac=$2} END{print mac}' | tr -d ':' | tr '[:lower:]' '[:upper:]')"

# Peer MAC written in the csv
PEER_MAC="$(awk -F, '$1=="mac"{print $4}' "$CSV_FILE_PATH" \
    | tr -d ': \r' | tr '[:lower:]' '[:upper:]')"

if [ -z "$BOARD_MAC" ] || [ -z "$PEER_MAC" ]; then
    echo "Error: could not read board MAC or peer MAC from csv."
    exit 1
fi

if [ "$BOARD_MAC" == "$PEER_MAC" ]; then
    echo "Error: csv peer MAC ($PEER_MAC) is this board's own MAC. Wrong csv?"
    exit 1
fi

echo "Board $BOARD_MAC -> peer $PEER_MAC"

BIN_FILE_PATH="$(mktemp --suffix=.bin)"
trap 'rm -f "$BIN_FILE_PATH"' EXIT

# Get partion size from `partitions.csv`
PARTITION_SIZE="$(awk -F, '$1=="peer_cfg"{gsub(/ /,"",$5); print $5}' partitions.csv)"

python "$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py" \
    generate "$CSV_FILE_PATH" "$BIN_FILE_PATH" "$PARTITION_SIZE"

python "$IDF_PATH/components/partition_table/parttool.py" \
    --port "$ESP_PORT" write_partition --partition-name=peer_cfg --input="$BIN_FILE_PATH"