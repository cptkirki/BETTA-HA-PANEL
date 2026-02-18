<img src="images/BETTAOS.jpg" alt="BETTA OS Logo" width="20%" />
# BETTA HA Panel 
A runtime configurable Home Assistant dashboard for the ESP32-P4 Smart 86 Box development board.

## Projektbeschreibung
BETTA HA Panel turns the Smart86 Box into a standalone 720x720 Home Assistant wall panel. It is built for a dedicated touchscreen experience with fast access to your most important entities, scenes, and automations.

The dashboard is configured directly on the device via the integrated BETTA Editor. Layout and settings are stored as JSON in LittleFS, so you can iterate quickly without rebuilding firmware for every UI change.

- Live connection to Home Assistant via WebSocket (optionally with REST fallback)
- Local web editor at `http://<panel-ip>` for layout, widgets, and settings
- Integrated provisioning flow for Wi-Fi and Home Assistant (including the setup AP `BETTA-Setup`)
- Multi-page widget dashboard, e.g., sensor, button, slider, graph, light, heating, and weather tiles
- Subtle Lottie-animated weather icons for a more dynamic forecast view

A few examples:

<p>
  <img src="images/light%20on%20example.jpg" alt="Light tiles ON example" width="49%" />
  <img src="images/heating%20on%20example.jpg" alt="Heating ON example" width="49%" />
</p>



![alt text](https://github.com/cptkirki/BETTA-HA-PANEL/blob/main/images/weather%20forecast.jpg "3day weather forecast")


Configuration via webconfig in BETTA Editor:
<img width="358" height="873" alt="image" src="https://github.com/user-attachments/assets/8c05cce8-983f-4715-a1fb-38ebbcbee563" />


available Widgets:

<img width="319" height="130" alt="image" src="https://github.com/user-attachments/assets/1a248656-d01f-4585-a639-6d7dceafd08d" />

Widgets can be configured and placed (drag and drop) on the canvas:

<img width="1406" height="856" alt="image" src="https://github.com/user-attachments/assets/c5959f98-e151-4d9e-ac31-d636e9d65dcc" />

<img width="1426" height="766" alt="image" src="https://github.com/user-attachments/assets/9f8ba27f-943a-4e7b-8e18-ff7bf37bfea0" />



