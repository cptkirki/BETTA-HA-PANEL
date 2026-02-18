![alt text](https://github.com/cptkirki/BETTA-HA-PANEL/blob/main/images/BETTAOS.jpg "BETTA OS Logo")
# BETTA HA Panel 
A runtime configurable Home Assistant dashboard for the ESP32-P4 Smart 86 Box development board.

## Projektbeschreibung
BETTA HA Panel macht aus der Smart86 Box ein eigenstaendiges 720x720 Home-Assistant-Wandpanel. Das Dashboard wird direkt auf dem Geraet ueber den integrierten BETTA Editor konfiguriert, waehrend Layout und Einstellungen als JSON in LittleFS gespeichert werden.

- Live-Anbindung an Home Assistant per WebSocket (optional mit REST-Fallback)
- Lokaler Web-Editor unter `http://<panel-ip>` fuer Layout, Widgets und Einstellungen
- Integrierter Provisioning-Flow fuer Wi-Fi und Home Assistant (inkl. Setup-AP `BETTA-Setup`)
- Mehrseitiges Widget-Dashboard, z. B. Sensor, Button, Slider, Graph, Light-, Heating- und Weather-Tiles

Ein paar Beispiele:

![alt text](https://github.com/cptkirki/BETTA-HA-PANEL/blob/main/images/light%20example.jpg "Light tile example")


![alt text](https://github.com/cptkirki/BETTA-HA-PANEL/blob/main/images/heating%20heating.jpg "Heating tile example")


![alt text](https://github.com/cptkirki/BETTA-HA-PANEL/blob/main/images/weather%20forecast.jpg "3day weather forecast")


Configuration via webconfig in BETTA Editor:
available Widgets:

<img width="319" height="130" alt="image" src="https://github.com/user-attachments/assets/1a248656-d01f-4585-a639-6d7dceafd08d" />

Widgets can be configured and placed on the canvas:

<img width="1406" height="856" alt="image" src="https://github.com/user-attachments/assets/c5959f98-e151-4d9e-ac31-d636e9d65dcc" />

<img width="1426" height="766" alt="image" src="https://github.com/user-attachments/assets/9f8ba27f-943a-4e7b-8e18-ff7bf37bfea0" />


<img width="358" height="873" alt="image" src="https://github.com/user-attachments/assets/8c05cce8-983f-4715-a1fb-38ebbcbee563" />


