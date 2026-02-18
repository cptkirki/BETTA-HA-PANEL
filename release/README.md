## ESP-Hosted C6 Firmware Source

The file `network_adapter_esp32c6_2.11.7.bin` in this folder is taken from the official ESP-Hosted firmware release:

- https://github.com/esphome/esp-hosted-firmware/releases/tag/v2.11.7

SHA256:

- `2AA46DBCD18F5B2046F58B82CDC62BDF21AF137930E67C4B3B114F6E3E0DAF15`

Project policy:

- Use C6 firmware only from `release/`.
- Do not auto-build C6 firmware in this repository.
- Trigger C6 OTA only when running version mismatches host version (including `0.0.0`).
