<!-- SPDX-License-Identifier: LicenseRef-FNCL-1.1 | Copyright (c) 2026 Christopher Gleiche -->
## ESP-Hosted C6 Firmware Source

The original C6 adapter firmware used for this release was:

- https://github.com/esphome/esp-hosted-firmware/releases/tag/v2.11.7

SHA256:

- `2AA46DBCD18F5B2046F58B82CDC62BDF21AF137930E67C4B3B114F6E3E0DAF15`

Important:

- The C6 `network_adapter` firmware is already embedded into the generated factory image `betta86-ha-panel.factory.bin`.
- For flashing/distribution, the factory image is sufficient.
- Keeping `network_adapter_esp32c6*.bin` in the repo is optional and not required for release delivery.

Project policy (build/runtime):

- Use C6 firmware only from `release/`.
- Do not auto-build C6 firmware in this repository.
- Trigger C6 OTA only when running version mismatches host version (including `0.0.0`).
