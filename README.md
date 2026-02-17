# BETTA HA Panel
A runtime configurable Home Assistant dashboard for the ESP32-P4 Smart 86 Box development board.

## Projektbeschreibung
BETTA HA Panel macht aus der Smart86 Box ein eigenstaendiges 720x720 Home-Assistant-Wandpanel. Das Dashboard wird direkt auf dem Geraet ueber den integrierten BETTA Editor konfiguriert, waehrend Layout und Einstellungen als JSON in LittleFS gespeichert werden.

- Live-Anbindung an Home Assistant per WebSocket (optional mit REST-Fallback)
- Lokaler Web-Editor unter `http://<panel-ip>` fuer Layout, Widgets und Einstellungen
- Integrierter Provisioning-Flow fuer Wi-Fi und Home Assistant (inkl. Setup-AP `BETTA-Setup`)
- Mehrseitiges Widget-Dashboard, z. B. Sensor, Button, Slider, Graph, Light-, Heating- und Weather-Tiles
