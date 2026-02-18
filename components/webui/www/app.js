const GRID = 10;
const CANVAS_WIDTH = 720;
const CANVAS_HEIGHT = 600;
const MIN_WIDGET_SIZE = 60;
const DEFAULT_SLIDER_DIRECTION = "auto";
const DEFAULT_BUTTON_MODE = "auto";
const DEFAULT_BUTTON_ACCENT_COLOR = "#6fe8ff";
const DEFAULT_SLIDER_ACCENT_COLOR = "#6fe8ff";
const DEFAULT_GRAPH_LINE_COLOR = "#6fe8ff";
const DEFAULT_GRAPH_TIME_WINDOW_MIN = 120;
const GRAPH_POINTS_MIN = 16;
const GRAPH_POINTS_MAX = 64;
const GRAPH_TIME_WINDOW_MIN = 1;
const GRAPH_TIME_WINDOW_MAX = 1440;
const ENTITY_AUTOCOMPLETE_DEBOUNCE_MS = 220;
const ENTITY_AUTOCOMPLETE_MAX_ITEMS = 24;
const DEFAULT_SLIDER_ENTITY_DOMAIN = "auto";
const SLIDER_DIRECTIONS = new Set([
  "auto",
  "left_to_right",
  "right_to_left",
  "bottom_to_top",
  "top_to_bottom",
]);
const SLIDER_ENTITY_DOMAINS = new Set([
  "auto",
  "light",
  "media_player",
  "cover",
]);
const BUTTON_MODES = new Set([
  "auto",
  "play_pause",
  "stop",
  "next",
  "previous",
]);
const LANGUAGE_CODE_RE = /^[a-z0-9][a-z0-9_-]{1,14}$/;
const DEFAULT_UI_LANGUAGE = "de";

const WEB_I18N_BUILTIN = {
  en: {
    "tabs.layout": "Layout",
    "tabs.settings": "Settings",
    "sidebar.title": "BETTA Editor",
    "sidebar.subtitle": "Layout source of truth: JSON",
    "provision.wifi.title": "Wi-Fi Provisioning",
    "provision.wifi.subtitle": "Connect the panel to your Wi-Fi.",
    "provision.wifi.ssid": "SSID",
    "provision.wifi.country_code": "Country Code",
    "provision.wifi.password": "Password",
    "provision.wifi.show_password": "Show password",
    "provision.ha.title": "HA Provisioning",
    "provision.ha.subtitle": "Connect the panel to Home Assistant.",
    "provision.ha.ws_url": "WebSocket URL (ws:// or wss://)",
    "provision.ha.token": "Long-lived Access Token",
    "provision.ha.show_token": "Show token",
    "settings.wifi.heading": "Wi-Fi",
    "settings.wifi.ssid": "SSID",
    "settings.wifi.country_code": "Country Code",
    "settings.wifi.bssid": "BSSID lock (optional)",
    "settings.wifi.password": "Password",
    "settings.ha.heading": "Home Assistant",
    "settings.ha.ws_url": "WebSocket URL (ws:// or wss://)",
    "settings.ha.token": "Long-lived Access Token",
    "settings.ha.rest_fallback": "Enable HA REST fallback (Default: Off, WS-only preferred)",
    "settings.time.heading": "Time",
    "settings.time.ntp_server": "NTP Server",
    "settings.time.timezone": "Timezone (POSIX TZ)",
    "settings.ui.heading": "UI",
    "settings.ui.language": "Language",
    "settings.ui.reload_languages": "Reload Languages",
    "settings.ui.download_json": "Download JSON",
    "settings.ui.upload_code": "Language Code",
    "settings.ui.upload_file": "Translation JSON File",
    "settings.ui.upload_button": "Upload / Add Language",
    "settings.ap.heading": "Setup AP",
    "settings.ap.hint": "If setup AP is active, connect to it and open <code>http://192.168.4.1</code>.",
    "settings.actions.heading": "Settings Actions",
    "settings.actions.reload": "Reload Settings",
    "settings.actions.save": "Save + Reboot",
    "settings.actions.hint": "After save, the device reboots and may switch from setup AP to your home Wi-Fi.",
    "settings.info.configured": "Configured",
    "settings.info.connected": "Connected",
    "settings.info.password_stored": "Password stored",
    "settings.info.country": "Country",
    "settings.info.rssi": "RSSI (connected AP)",
    "settings.info.connected_bssid": "Connected BSSID",
    "settings.info.channel": "Channel",
    "settings.info.token_stored": "Token stored",
    "settings.info.rest_fallback": "REST fallback",
    "common.yes": "yes",
    "common.no": "no",
    "common.scan": "Scan",
    "common.scan_wifi": "Scan Wi-Fi",
    "common.save_reboot": "Save + Reboot",
    "status.idle": "Idle",
    "status.loading_settings": "Loading settings...",
    "status.settings_loaded": "Settings loaded",
    "status.settings_load_failed": "Settings load failed: {error}",
    "status.saving_settings": "Saving settings...",
    "status.settings_saved_reboot": "Settings saved. Device reboots in ~2s. Reconnect and reopen the panel URL.",
    "status.wifi_scan_running": "Scanning Wi-Fi...",
    "status.wifi_scan_complete": "Wi-Fi scan complete ({count} networks)",
    "status.wifi_scan_failed": "Wi-Fi scan failed: {error}",
    "wifi.scan_unavailable": "Wi-Fi scan is unavailable in setup AP mode on this hardware. Enter SSID manually.",
    "wifi.scan_click": "Click \"Scan Wi-Fi\" to list nearby networks.",
    "wifi.scan_click_short": "Click \"Scan\" to list nearby networks.",
    "wifi.scan_no_networks": "No networks found. Move closer to your router and scan again.",
    "wifi.scan_found": "{count} network(s) found. Select one to fill SSID.",
    "wifi.scan.connected_tag": "connected",
    "wifi.scan.option_unavailable": "Scan unavailable",
    "wifi.scan.option_scanning": "Scanning...",
    "wifi.scan.option_not_run": "No scan yet",
    "wifi.scan.option_no_networks": "No networks found",
    "wifi.scan.option_select": "Select network ({count} found)",
    "settings.time.info": "Applied after reboot. Time sync starts when Wi-Fi is connected.",
    "settings.ui.info": "Applied after reboot.",
    "settings.ap.active": "Setup AP active: {ssid}\\nOpen http://192.168.4.1 while connected to this AP.",
    "settings.ap.inactive": "Setup AP inactive.\\nUse the panel IP in your home Wi-Fi network.",
    "settings.translation.info": "Upload a JSON file to add or update a language.",
    "settings.translation.upload_ok": "Language \"{lang}\" uploaded.",
    "settings.translation.upload_fail": "Upload failed: {error}",
    "settings.translation.no_file": "Choose a JSON file first.",
    "settings.translation.invalid_code": "Language code must use [a-z0-9_-] and be 2-15 chars.",
    "settings.language.invalid_country": "Wi-Fi country code must be a 2-letter ISO code (e.g. US, DE)",
    "settings.language.invalid_bssid": "BSSID must be empty or in format AA:BB:CC:DD:EE:FF",
    "settings.language.invalid_ha_url": "HA URL must start with ws:// or wss://",
    "provision.wifi.required_ssid": "SSID is required.",
    "provision.wifi.required_country": "Country code must be 2 letters (e.g. US, DE).",
    "provision.ha.required_url": "WebSocket URL is required.",
    "provision.ha.invalid_url": "HA URL must start with ws:// or wss://.",
    "provision.ha.required_token": "Long-lived Access Token is required.",
    "provision.saving_reboot": "Saving settings and rebooting...",
    "provision.saved_reboot": "Settings saved. Device reboots in ~2s.",
    "provision.wifi.hint": "Save reboots the panel. After reboot, HA provisioning is shown.",
    "provision.ha.hint": "Save reboots the panel. After reboot, the editor is unlocked.",
    "settings.language.option_de": "Deutsch",
    "settings.language.option_en": "English",
    "settings.language.option_es": "Espanol",
  },
  de: {
    "tabs.layout": "Layout",
    "tabs.settings": "Einstellungen",
    "sidebar.title": "BETTA Editor",
    "sidebar.subtitle": "Layout Quelle: JSON",
    "provision.wifi.title": "WLAN Provisioning",
    "provision.wifi.subtitle": "Verbinde das Panel mit deinem WLAN.",
    "provision.wifi.ssid": "SSID",
    "provision.wifi.country_code": "Laendercode",
    "provision.wifi.password": "Passwort",
    "provision.wifi.show_password": "Passwort anzeigen",
    "provision.ha.title": "HA Provisioning",
    "provision.ha.subtitle": "Verbinde das Panel mit Home Assistant.",
    "provision.ha.ws_url": "WebSocket URL (ws:// oder wss://)",
    "provision.ha.token": "Long-lived Access Token",
    "provision.ha.show_token": "Token anzeigen",
    "settings.wifi.heading": "WLAN",
    "settings.wifi.ssid": "SSID",
    "settings.wifi.country_code": "Laendercode",
    "settings.wifi.bssid": "BSSID Lock (optional)",
    "settings.wifi.password": "Passwort",
    "settings.ha.heading": "Home Assistant",
    "settings.ha.ws_url": "WebSocket URL (ws:// oder wss://)",
    "settings.ha.token": "Long-lived Access Token",
    "settings.ha.rest_fallback": "HA REST Fallback aktivieren (Standard: Aus, WS bevorzugt)",
    "settings.time.heading": "Zeit",
    "settings.time.ntp_server": "NTP Server",
    "settings.time.timezone": "Zeitzone (POSIX TZ)",
    "settings.ui.heading": "UI",
    "settings.ui.language": "Sprache",
    "settings.ui.reload_languages": "Sprachen neu laden",
    "settings.ui.download_json": "JSON herunterladen",
    "settings.ui.upload_code": "Sprachcode",
    "settings.ui.upload_file": "Uebersetzungsdatei (JSON)",
    "settings.ui.upload_button": "Upload / Sprache hinzufuegen",
    "settings.ap.heading": "Setup AP",
    "settings.ap.hint": "Wenn Setup AP aktiv ist, verbinden und <code>http://192.168.4.1</code> oeffnen.",
    "settings.actions.heading": "Einstellungsaktionen",
    "settings.actions.reload": "Einstellungen neu laden",
    "settings.actions.save": "Speichern + Neustart",
    "settings.actions.hint": "Nach dem Speichern startet das Geraet neu und wechselt ggf. vom Setup AP ins Heim-WLAN.",
    "settings.info.configured": "Konfiguriert",
    "settings.info.connected": "Verbunden",
    "settings.info.password_stored": "Passwort gespeichert",
    "settings.info.country": "Land",
    "settings.info.rssi": "RSSI (verbundener AP)",
    "settings.info.connected_bssid": "Verbundener BSSID",
    "settings.info.channel": "Kanal",
    "settings.info.token_stored": "Token gespeichert",
    "settings.info.rest_fallback": "REST Fallback",
    "common.yes": "ja",
    "common.no": "nein",
    "common.scan": "Scan",
    "common.scan_wifi": "WLAN scannen",
    "common.save_reboot": "Speichern + Neustart",
    "status.idle": "Bereit",
    "status.loading_settings": "Einstellungen werden geladen...",
    "status.settings_loaded": "Einstellungen geladen",
    "status.settings_load_failed": "Einstellungen laden fehlgeschlagen: {error}",
    "status.saving_settings": "Einstellungen werden gespeichert...",
    "status.settings_saved_reboot": "Einstellungen gespeichert. Das Geraet startet in ~2s neu.",
    "status.wifi_scan_running": "WLAN Suche laeuft...",
    "status.wifi_scan_complete": "WLAN Scan fertig ({count} Netze)",
    "status.wifi_scan_failed": "WLAN Scan fehlgeschlagen: {error}",
    "wifi.scan_unavailable": "WLAN Scan ist im Setup AP Modus auf dieser Hardware nicht verfuegbar. SSID manuell eingeben.",
    "wifi.scan_click": "Auf \"WLAN scannen\" klicken, um Netze zu finden.",
    "wifi.scan_click_short": "Auf \"Scan\" klicken, um Netze zu finden.",
    "wifi.scan_no_networks": "Keine Netze gefunden. Gehe naeher an den Router und versuche es erneut.",
    "wifi.scan_found": "{count} Netzwerk(e) gefunden. SSID auswaehlen.",
    "wifi.scan.connected_tag": "verbunden",
    "wifi.scan.option_unavailable": "Scan nicht verfuegbar",
    "wifi.scan.option_scanning": "Suche laeuft...",
    "wifi.scan.option_not_run": "Noch kein Scan",
    "wifi.scan.option_no_networks": "Keine Netze gefunden",
    "wifi.scan.option_select": "Netz waehlen ({count} gefunden)",
    "settings.time.info": "Wird nach Neustart angewendet. Zeitsync startet bei WLAN Verbindung.",
    "settings.ui.info": "Wird nach Neustart angewendet.",
    "settings.ap.active": "Setup AP aktiv: {ssid}\\nhttp://192.168.4.1 im AP oeffnen.",
    "settings.ap.inactive": "Setup AP inaktiv.\\nNutze die Panel-IP im Heimnetz.",
    "settings.translation.info": "JSON hochladen, um eine Sprache hinzuzufuegen oder zu aktualisieren.",
    "settings.translation.upload_ok": "Sprache \"{lang}\" hochgeladen.",
    "settings.translation.upload_fail": "Upload fehlgeschlagen: {error}",
    "settings.translation.no_file": "Bitte zuerst eine JSON Datei auswaehlen.",
    "settings.translation.invalid_code": "Sprachcode muss [a-z0-9_-] nutzen und 2-15 Zeichen haben.",
    "settings.language.invalid_country": "WLAN Laendercode muss ein 2-stelliger ISO Code sein (z.B. US, DE)",
    "settings.language.invalid_bssid": "BSSID muss leer sein oder Format AA:BB:CC:DD:EE:FF haben",
    "settings.language.invalid_ha_url": "HA URL muss mit ws:// oder wss:// beginnen",
    "provision.wifi.required_ssid": "SSID ist erforderlich.",
    "provision.wifi.required_country": "Laendercode muss 2 Buchstaben haben (z.B. US, DE).",
    "provision.ha.required_url": "WebSocket URL ist erforderlich.",
    "provision.ha.invalid_url": "HA URL muss mit ws:// oder wss:// beginnen.",
    "provision.ha.required_token": "Long-lived Access Token ist erforderlich.",
    "provision.saving_reboot": "Speichere Einstellungen und starte neu...",
    "provision.saved_reboot": "Einstellungen gespeichert. Geraet startet in ~2s neu.",
    "provision.wifi.hint": "Speichern startet das Panel neu. Danach folgt die HA Einrichtung.",
    "provision.ha.hint": "Speichern startet das Panel neu. Danach ist der Editor freigeschaltet.",
    "settings.language.option_de": "Deutsch",
    "settings.language.option_en": "Englisch",
    "settings.language.option_es": "Spanisch",
  },
  es: {
    "tabs.layout": "Diseno",
    "tabs.settings": "Configuracion",
    "sidebar.title": "BETTA Editor",
    "sidebar.subtitle": "Fuente de verdad del layout: JSON",
    "provision.wifi.title": "Provision Wi-Fi",
    "provision.wifi.subtitle": "Conecta el panel a tu red Wi-Fi.",
    "provision.wifi.ssid": "SSID",
    "provision.wifi.country_code": "Codigo de pais",
    "provision.wifi.password": "Contrasena",
    "provision.wifi.show_password": "Mostrar contrasena",
    "provision.ha.title": "Provision HA",
    "provision.ha.subtitle": "Conecta el panel a Home Assistant.",
    "provision.ha.ws_url": "URL WebSocket (ws:// o wss://)",
    "provision.ha.token": "Token de acceso de larga duracion",
    "provision.ha.show_token": "Mostrar token",
    "settings.wifi.heading": "Wi-Fi",
    "settings.wifi.ssid": "SSID",
    "settings.wifi.country_code": "Codigo de pais",
    "settings.wifi.bssid": "Bloqueo BSSID (opcional)",
    "settings.wifi.password": "Contrasena",
    "settings.ha.heading": "Home Assistant",
    "settings.ha.ws_url": "URL WebSocket (ws:// o wss://)",
    "settings.ha.token": "Token de acceso de larga duracion",
    "settings.ha.rest_fallback": "Activar fallback REST de HA (por defecto: off, se prefiere WS)",
    "settings.time.heading": "Hora",
    "settings.time.ntp_server": "Servidor NTP",
    "settings.time.timezone": "Zona horaria (POSIX TZ)",
    "settings.ui.heading": "UI",
    "settings.ui.language": "Idioma",
    "settings.ui.reload_languages": "Recargar idiomas",
    "settings.ui.download_json": "Descargar JSON",
    "settings.ui.upload_code": "Codigo de idioma",
    "settings.ui.upload_file": "Archivo JSON de traduccion",
    "settings.ui.upload_button": "Subir / Agregar idioma",
    "settings.ap.heading": "AP de setup",
    "settings.ap.hint": "Si el AP de setup esta activo, conectate y abre <code>http://192.168.4.1</code>.",
    "settings.actions.heading": "Acciones de configuracion",
    "settings.actions.reload": "Recargar configuracion",
    "settings.actions.save": "Guardar + Reiniciar",
    "settings.actions.hint": "Despues de guardar, el dispositivo reinicia y puede cambiar del AP de setup al Wi-Fi de casa.",
    "settings.info.configured": "Configurado",
    "settings.info.connected": "Conectado",
    "settings.info.password_stored": "Contrasena guardada",
    "settings.info.country": "Pais",
    "settings.info.rssi": "RSSI (AP conectado)",
    "settings.info.connected_bssid": "BSSID conectado",
    "settings.info.channel": "Canal",
    "settings.info.token_stored": "Token guardado",
    "settings.info.rest_fallback": "Fallback REST",
    "common.yes": "si",
    "common.no": "no",
    "common.scan": "Escanear",
    "common.scan_wifi": "Escanear Wi-Fi",
    "common.save_reboot": "Guardar + Reiniciar",
    "status.idle": "Listo",
    "status.loading_settings": "Cargando configuracion...",
    "status.settings_loaded": "Configuracion cargada",
    "status.settings_load_failed": "Error al cargar configuracion: {error}",
    "status.saving_settings": "Guardando configuracion...",
    "status.settings_saved_reboot": "Configuracion guardada. El dispositivo reinicia en ~2s.",
    "status.wifi_scan_running": "Escaneo Wi-Fi en curso...",
    "status.wifi_scan_complete": "Escaneo Wi-Fi completo ({count} redes)",
    "status.wifi_scan_failed": "Error en escaneo Wi-Fi: {error}",
    "wifi.scan_unavailable": "El escaneo Wi-Fi no esta disponible en modo setup AP en este hardware. Ingresa SSID manualmente.",
    "wifi.scan_click": "Pulsa \"Escanear Wi-Fi\" para listar redes cercanas.",
    "wifi.scan_click_short": "Pulsa \"Escanear\" para listar redes cercanas.",
    "wifi.scan_no_networks": "No se encontraron redes. Acercate al router e intenta de nuevo.",
    "wifi.scan_found": "{count} red(es) encontradas. Selecciona una para llenar SSID.",
    "wifi.scan.connected_tag": "conectado",
    "wifi.scan.option_unavailable": "Escaneo no disponible",
    "wifi.scan.option_scanning": "Escaneando...",
    "wifi.scan.option_not_run": "Sin escaneo",
    "wifi.scan.option_no_networks": "No se encontraron redes",
    "wifi.scan.option_select": "Selecciona red ({count} encontradas)",
    "settings.time.info": "Se aplica despues de reiniciar. La sincronizacion inicia cuando Wi-Fi esta conectado.",
    "settings.ui.info": "Se aplica despues de reiniciar.",
    "settings.ap.active": "AP de setup activo: {ssid}\\nAbre http://192.168.4.1 conectado a este AP.",
    "settings.ap.inactive": "AP de setup inactivo.\\nUsa la IP del panel en tu Wi-Fi.",
    "settings.translation.info": "Sube un JSON para agregar o actualizar un idioma.",
    "settings.translation.upload_ok": "Idioma \"{lang}\" subido.",
    "settings.translation.upload_fail": "Error de subida: {error}",
    "settings.translation.no_file": "Selecciona primero un archivo JSON.",
    "settings.translation.invalid_code": "El codigo de idioma debe usar [a-z0-9_-] y tener 2-15 caracteres.",
    "settings.language.invalid_country": "El codigo de pais Wi-Fi debe ser ISO de 2 letras (p.ej. US, DE)",
    "settings.language.invalid_bssid": "El BSSID debe estar vacio o en formato AA:BB:CC:DD:EE:FF",
    "settings.language.invalid_ha_url": "La URL HA debe empezar con ws:// o wss://",
    "provision.wifi.required_ssid": "SSID es obligatorio.",
    "provision.wifi.required_country": "El codigo de pais debe tener 2 letras (p.ej. US, DE).",
    "provision.ha.required_url": "La URL WebSocket es obligatoria.",
    "provision.ha.invalid_url": "La URL HA debe empezar con ws:// o wss://.",
    "provision.ha.required_token": "El token de acceso es obligatorio.",
    "provision.saving_reboot": "Guardando configuracion y reiniciando...",
    "provision.saved_reboot": "Configuracion guardada. El dispositivo reinicia en ~2s.",
    "provision.wifi.hint": "Guardar reinicia el panel. Despues se muestra la provision de HA.",
    "provision.ha.hint": "Guardar reinicia el panel. Despues se desbloquea el editor.",
    "settings.language.option_de": "Aleman",
    "settings.language.option_en": "Ingles",
    "settings.language.option_es": "Espanol",
  },
};

function widgetSizeLimits(type) {
  const fallback = {
    minW: MIN_WIDGET_SIZE,
    minH: MIN_WIDGET_SIZE,
    maxW: CANVAS_WIDTH,
    maxH: CANVAS_HEIGHT,
  };

  switch (type) {
    case "sensor":
      return { minW: 120, minH: 80, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "button":
      return { minW: 100, minH: 100, maxW: 480, maxH: 320 };
    case "slider":
      return { minW: 100, minH: 100, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "graph":
      return { minW: 220, minH: 140, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "empty_tile":
      return { minW: 120, minH: 80, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "light_tile":
      return { minW: 180, minH: 180, maxW: 480, maxH: 480 };
    case "heating_tile":
      return { minW: 220, minH: 200, maxW: 480, maxH: 480 };
    case "weather_tile":
      return { minW: 220, minH: 200, maxW: 480, maxH: 480 };
    case "weather_3day":
      return { minW: 260, minH: 220, maxW: 640, maxH: 420 };
    default:
      return fallback;
  }
}

function clampRectToCanvas(rect, type) {
  const limits = widgetSizeLimits(type);
  const maxW = Math.min(limits.maxW, CANVAS_WIDTH);
  const maxH = Math.min(limits.maxH, CANVAS_HEIGHT);
  const minW = Math.min(limits.minW, maxW);
  const minH = Math.min(limits.minH, maxH);

  const w = clamp(snap(Number(rect.w || minW)), minW, maxW);
  const h = clamp(snap(Number(rect.h || minH)), minH, maxH);
  const x = clamp(snap(Number(rect.x || 0)), 0, CANVAS_WIDTH - w);
  const y = clamp(snap(Number(rect.y || 0)), 0, CANVAS_HEIGHT - h);

  return { x, y, w, h };
}

const editor = {
  layout: null,
  entities: [],
  states: new Map(),
  selectedPageId: null,
  selectedWidgetId: null,
  activePane: "layout",
  provisioningStage: null,
  editorStarted: false,
  settings: null,
  wifiScanItems: [],
  wifiScanHasRun: false,
  wifiScanInProgress: false,
  wifiScanSupported: true,
  languageCatalog: [],
  i18nLanguage: DEFAULT_UI_LANGUAGE,
  i18nMap: { ...(WEB_I18N_BUILTIN.en || {}) },
  i18nEffective: {},
  sectionCollapsed: {
    pages: false,
    widgets: false,
    inspector: false,
  },
};

const el = {
  provisioningRoot: document.getElementById("provisioningRoot"),
  provisioningWifiPage: document.getElementById("provisioningWifiPage"),
  provisioningHaPage: document.getElementById("provisioningHaPage"),
  editorShell: document.getElementById("editorShell"),
  layoutTabBtn: document.getElementById("layoutTabBtn"),
  settingsTabBtn: document.getElementById("settingsTabBtn"),
  layoutPane: document.getElementById("layoutPane"),
  settingsPane: document.getElementById("settingsPane"),
  pagesList: document.getElementById("pagesList"),
  pagesMiniList: document.getElementById("pagesMiniList"),
  widgetsList: document.getElementById("widgetsList"),
  canvas: document.getElementById("canvas"),
  canvasTitle: document.getElementById("canvasTitle"),
  status: document.getElementById("status"),
  pagesSection: document.getElementById("pagesSection"),
  widgetsSection: document.getElementById("widgetsSection"),
  inspectorSection: document.getElementById("inspectorSection"),
  togglePagesSection: document.getElementById("togglePagesSection"),
  toggleWidgetsSection: document.getElementById("toggleWidgetsSection"),
  toggleInspectorSection: document.getElementById("toggleInspectorSection"),
  addPageBtn: document.getElementById("addPageBtn"),
  deletePageBtn: document.getElementById("deletePageBtn"),
  pageTitleInput: document.getElementById("pageTitleInput"),
  applyPageBtn: document.getElementById("applyPageBtn"),
  addSensorBtn: document.getElementById("addSensorBtn"),
  addButtonBtn: document.getElementById("addButtonBtn"),
  addSliderBtn: document.getElementById("addSliderBtn"),
  addGraphBtn: document.getElementById("addGraphBtn"),
  addEmptyTileBtn: document.getElementById("addEmptyTileBtn"),
  addLightTileBtn: document.getElementById("addLightTileBtn"),
  addHeatingTileBtn: document.getElementById("addHeatingTileBtn"),
  addWeatherTileBtn: document.getElementById("addWeatherTileBtn"),
  addWeather3DayBtn: document.getElementById("addWeather3DayBtn"),
  deleteWidgetBtn: document.getElementById("deleteWidgetBtn"),
  reloadBtn: document.getElementById("reloadBtn"),
  saveBtn: document.getElementById("saveBtn"),
  exportBtn: document.getElementById("exportBtn"),
  importBtn: document.getElementById("importBtn"),
  importFile: document.getElementById("importFile"),
  jsonPaste: document.getElementById("jsonPaste"),
  fTitle: document.getElementById("fTitle"),
  fType: document.getElementById("fType"),
  fEntityWrap: document.getElementById("fEntityWrap"),
  fEntity: document.getElementById("fEntity"),
  fSecondaryEntityWrap: document.getElementById("fSecondaryEntityWrap"),
  fSecondaryEntity: document.getElementById("fSecondaryEntity"),
  buttonOptions: document.getElementById("buttonOptions"),
  fButtonMode: document.getElementById("fButtonMode"),
  fSliderEntityDomain: document.getElementById("fSliderEntityDomain"),
  fButtonAccentColor: document.getElementById("fButtonAccentColor"),
  sliderOptions: document.getElementById("sliderOptions"),
  fSliderDirection: document.getElementById("fSliderDirection"),
  fSliderAccentColor: document.getElementById("fSliderAccentColor"),
  graphOptions: document.getElementById("graphOptions"),
  fGraphLineColor: document.getElementById("fGraphLineColor"),
  fGraphTimeWindowMin: document.getElementById("fGraphTimeWindowMin"),
  fGraphPointCount: document.getElementById("fGraphPointCount"),
  fX: document.getElementById("fX"),
  fY: document.getElementById("fY"),
  fW: document.getElementById("fW"),
  fH: document.getElementById("fH"),
  applyInspectorBtn: document.getElementById("applyInspectorBtn"),
  entityOptions: document.getElementById("entityOptions"),
  sensorEntityOptions: document.getElementById("sensorEntityOptions"),
  settingsWifiSsid: document.getElementById("settingsWifiSsid"),
  settingsWifiCountryCode: document.getElementById("settingsWifiCountryCode"),
  settingsWifiBssid: document.getElementById("settingsWifiBssid"),
  scanWifiBtn: document.getElementById("scanWifiBtn"),
  settingsWifiScanResults: document.getElementById("settingsWifiScanResults"),
  settingsWifiScanInfo: document.getElementById("settingsWifiScanInfo"),
  settingsWifiPassword: document.getElementById("settingsWifiPassword"),
  settingsHaUrl: document.getElementById("settingsHaUrl"),
  settingsHaToken: document.getElementById("settingsHaToken"),
  settingsHaRestEnabled: document.getElementById("settingsHaRestEnabled"),
  settingsNtpServer: document.getElementById("settingsNtpServer"),
  settingsTimezone: document.getElementById("settingsTimezone"),
  settingsLanguage: document.getElementById("settingsLanguage"),
  reloadLanguagesBtn: document.getElementById("reloadLanguagesBtn"),
  downloadLanguageBtn: document.getElementById("downloadLanguageBtn"),
  uploadLanguageCode: document.getElementById("uploadLanguageCode"),
  uploadLanguageFile: document.getElementById("uploadLanguageFile"),
  uploadLanguageBtn: document.getElementById("uploadLanguageBtn"),
  settingsTranslationInfo: document.getElementById("settingsTranslationInfo"),
  settingsWifiInfo: document.getElementById("settingsWifiInfo"),
  settingsHaInfo: document.getElementById("settingsHaInfo"),
  settingsTimeInfo: document.getElementById("settingsTimeInfo"),
  settingsUiInfo: document.getElementById("settingsUiInfo"),
  settingsApInfo: document.getElementById("settingsApInfo"),
  reloadSettingsBtn: document.getElementById("reloadSettingsBtn"),
  saveSettingsBtn: document.getElementById("saveSettingsBtn"),
  provWifiSsid: document.getElementById("provWifiSsid"),
  provWifiCountryCode: document.getElementById("provWifiCountryCode"),
  provScanWifiBtn: document.getElementById("provScanWifiBtn"),
  provWifiScanResults: document.getElementById("provWifiScanResults"),
  provWifiScanInfo: document.getElementById("provWifiScanInfo"),
  provWifiPassword: document.getElementById("provWifiPassword"),
  provWifiShowPassword: document.getElementById("provWifiShowPassword"),
  provWifiInfo: document.getElementById("provWifiInfo"),
  provWifiSaveBtn: document.getElementById("provWifiSaveBtn"),
  provHaUrl: document.getElementById("provHaUrl"),
  provHaToken: document.getElementById("provHaToken"),
  provHaShowToken: document.getElementById("provHaShowToken"),
  provHaInfo: document.getElementById("provHaInfo"),
  provHaSaveBtn: document.getElementById("provHaSaveBtn"),
};

const entityAutocomplete = {
  primary: {
    timerId: null,
    requestSeq: 0,
  },
  secondary: {
    timerId: null,
    requestSeq: 0,
  },
};

function normalizeSliderDirection(value) {
  return SLIDER_DIRECTIONS.has(value) ? value : DEFAULT_SLIDER_DIRECTION;
}

function normalizeSliderEntityDomain(value) {
  return SLIDER_ENTITY_DOMAINS.has(value) ? value : DEFAULT_SLIDER_ENTITY_DOMAIN;
}

function normalizeButtonMode(value) {
  return BUTTON_MODES.has(value) ? value : DEFAULT_BUTTON_MODE;
}

function buttonModeRequiresMediaPlayer(value) {
  const mode = normalizeButtonMode(value);
  return mode === "play_pause" || mode === "stop" || mode === "next" || mode === "previous";
}

function normalizeHexColor(value, fallback = DEFAULT_SLIDER_ACCENT_COLOR) {
  const source = (typeof value === "string" ? value : "").trim();
  const fallbackNorm = typeof fallback === "string" ? fallback.trim().toLowerCase() : DEFAULT_SLIDER_ACCENT_COLOR;
  if (!source) return fallbackNorm;

  let hex = source.toLowerCase();
  if (hex.startsWith("0x")) {
    hex = `#${hex.slice(2)}`;
  }
  if (!hex.startsWith("#")) {
    hex = `#${hex}`;
  }

  if (/^#[0-9a-f]{6}$/.test(hex)) {
    return hex;
  }
  return fallbackNorm;
}

function normalizeGraphPointCount(value) {
  if (value === null || value === undefined) return 0;
  if (typeof value === "string" && value.trim() === "") return 0;

  const parsed = Number(value);
  if (!Number.isFinite(parsed)) return 0;

  const rounded = Math.round(parsed);
  if (rounded <= 0) return 0;
  return clamp(rounded, GRAPH_POINTS_MIN, GRAPH_POINTS_MAX);
}

function normalizeGraphTimeWindowMin(value) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) return DEFAULT_GRAPH_TIME_WINDOW_MIN;
  const rounded = Math.round(parsed);
  if (rounded <= 0) return DEFAULT_GRAPH_TIME_WINDOW_MIN;
  return clamp(rounded, GRAPH_TIME_WINDOW_MIN, GRAPH_TIME_WINDOW_MAX);
}

function normalizeLayoutWidgets(layout) {
  if (!layout || !Array.isArray(layout.pages)) return;
  for (const page of layout.pages) {
    if (!page || !Array.isArray(page.widgets)) continue;
    for (const widget of page.widgets) {
      if (!widget || typeof widget !== "object") continue;
      if (widget.type === "button") {
        widget.button_accent_color = normalizeHexColor(widget.button_accent_color, DEFAULT_BUTTON_ACCENT_COLOR);
        const buttonMode = normalizeButtonMode(widget.button_mode);
        if (buttonModeRequiresMediaPlayer(buttonMode) && !String(widget.entity_id || "").startsWith("media_player.")) {
          widget.button_mode = DEFAULT_BUTTON_MODE;
        } else {
          widget.button_mode = buttonMode;
        }
      }
      if (widget.type === "slider") {
        widget.slider_direction = normalizeSliderDirection(widget.slider_direction);
        widget.slider_accent_color = normalizeHexColor(widget.slider_accent_color, DEFAULT_SLIDER_ACCENT_COLOR);
        widget.slider_entity_domain = normalizeSliderEntityDomain(widget.slider_entity_domain);
      }
      if (widget.type === "graph") {
        widget.graph_line_color = normalizeHexColor(widget.graph_line_color, DEFAULT_GRAPH_LINE_COLOR);
        widget.graph_time_window_min = normalizeGraphTimeWindowMin(widget.graph_time_window_min);
        const normalizedGraphPoints = normalizeGraphPointCount(widget.graph_point_count);
        if (normalizedGraphPoints > 0) {
          widget.graph_point_count = normalizedGraphPoints;
        } else {
          delete widget.graph_point_count;
        }
      }
    }
  }
}

function setStatus(text, isError = false) {
  el.status.textContent = text;
  el.status.style.color = isError ? "#ff8f94" : "#9db0c3";
}

function getWifiScanUi(scope = "settings") {
  if (scope === "provisioning") {
    return {
      scanButton: el.provScanWifiBtn,
      ssidInput: el.provWifiSsid,
      resultsSelect: el.provWifiScanResults,
      info: el.provWifiScanInfo,
    };
  }

  return {
    scanButton: el.scanWifiBtn,
    ssidInput: el.settingsWifiSsid,
    resultsSelect: el.settingsWifiScanResults,
    info: el.settingsWifiScanInfo,
  };
}

function setWifiScanInfo(text, isError = false, scope = "settings") {
  const ui = getWifiScanUi(scope);
  if (!ui.info) return;
  ui.info.textContent = text;
  ui.info.classList.toggle("error", isError);
}

function setProvisioningInfo(stage, text, isError = false) {
  const target = stage === "wifi" ? el.provWifiInfo : el.provHaInfo;
  if (!target) return;
  target.textContent = text;
  target.classList.toggle("error", isError);
}

function normalizeCountryCode(value) {
  const normalized = (value || "").trim().toUpperCase();
  return /^[A-Z]{2}$/.test(normalized) ? normalized : "";
}

function normalizeBssid(value) {
  const normalized = (value || "").trim().toUpperCase().replace(/-/g, ":");
  if (!normalized) return "";
  return /^[0-9A-F]{2}(:[0-9A-F]{2}){5}$/.test(normalized) ? normalized : "";
}

function normalizeLanguageCode(value, fallback = "") {
  const normalized = (typeof value === "string" ? value : "").trim().toLowerCase();
  if (!normalized) return fallback;
  return LANGUAGE_CODE_RE.test(normalized) ? normalized : fallback;
}

function normalizeUiLanguage(value) {
  return normalizeLanguageCode(value, DEFAULT_UI_LANGUAGE);
}

function templateString(text, vars = {}) {
  return String(text).replace(/\{([a-zA-Z0-9_]+)\}/g, (match, key) => {
    if (!Object.prototype.hasOwnProperty.call(vars, key)) return match;
    return String(vars[key]);
  });
}

function t(key, vars = {}, fallbackText = null) {
  const source = editor.i18nMap || {};
  const fallback = WEB_I18N_BUILTIN.en || {};
  const raw = source[key] ?? fallback[key] ?? fallbackText ?? key;
  return templateString(raw, vars);
}

function flattenTranslationObject(obj, prefix = "", out = {}) {
  if (!obj || typeof obj !== "object") return out;
  for (const [key, value] of Object.entries(obj)) {
    const path = prefix ? `${prefix}.${key}` : key;
    if (value && typeof value === "object" && !Array.isArray(value)) {
      flattenTranslationObject(value, path, out);
    } else if (typeof value === "string") {
      out[path] = value;
    }
  }
  return out;
}

function setTextById(id, key, vars) {
  const node = document.getElementById(id);
  if (!node) return;
  node.textContent = t(key, vars);
}

function setPlaceholderById(id, key, vars) {
  const node = document.getElementById(id);
  if (!node) return;
  node.placeholder = t(key, vars);
}

function renderLanguageOptions() {
  if (!el.settingsLanguage) return;

  const optionCodes = new Set();
  optionCodes.add(DEFAULT_UI_LANGUAGE);
  optionCodes.add("en");
  optionCodes.add("es");
  optionCodes.add(editor.i18nLanguage || DEFAULT_UI_LANGUAGE);
  for (const language of editor.languageCatalog || []) {
    const code = normalizeLanguageCode(language?.code, "");
    if (code) optionCodes.add(code);
  }

  const current = normalizeUiLanguage(el.settingsLanguage.value || editor.i18nLanguage);
  const sorted = Array.from(optionCodes).sort((a, b) => a.localeCompare(b));
  el.settingsLanguage.innerHTML = "";
  for (const code of sorted) {
    const option = document.createElement("option");
    option.value = code;
    const optionKey = `settings.language.option_${code}`;
    option.textContent = (editor.i18nMap && editor.i18nMap[optionKey]) || code.toUpperCase();
    el.settingsLanguage.appendChild(option);
  }
  el.settingsLanguage.value = sorted.includes(current) ? current : (sorted[0] || DEFAULT_UI_LANGUAGE);

  if (el.uploadLanguageCode) {
    el.uploadLanguageCode.value = el.settingsLanguage.value;
  }
}

async function loadLanguageCatalog() {
  const payload = await apiGet("/api/i18n/languages");
  if (!payload || !Array.isArray(payload.languages)) {
    throw new Error("Invalid language catalog");
  }
  editor.languageCatalog = payload.languages;
  return payload;
}

async function loadEffectiveTranslation(language) {
  const lang = normalizeUiLanguage(language);
  const response = await fetch(`/api/i18n/effective?lang=${encodeURIComponent(lang)}`, { cache: "no-store" });
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return response.json();
}

async function loadI18nLanguage(language, refreshCatalog = false) {
  const lang = normalizeUiLanguage(language);

  if (refreshCatalog || !Array.isArray(editor.languageCatalog) || editor.languageCatalog.length === 0) {
    try {
      await loadLanguageCatalog();
    } catch (_) {}
  }

  let effective = {};
  try {
    effective = await loadEffectiveTranslation(lang);
  } catch (_) {
    effective = {};
  }

  const builtin = WEB_I18N_BUILTIN[lang] || {};
  const webCustom = flattenTranslationObject(effective?.web || {});
  editor.i18nMap = {
    ...(WEB_I18N_BUILTIN.en || {}),
    ...builtin,
    ...webCustom,
  };
  editor.i18nEffective = effective || {};
  editor.i18nLanguage = lang;
  document.documentElement.lang = lang;

  applyWebTranslations();
  renderLanguageOptions();
}

function applyWebTranslations() {
  setTextById("layoutTabBtn", "tabs.layout");
  setTextById("settingsTabBtn", "tabs.settings");
  setTextById("sidebarTitle", "sidebar.title");
  setTextById("sidebarSubtitle", "sidebar.subtitle");

  setTextById("provWifiTitle", "provision.wifi.title");
  setTextById("provWifiSubtitle", "provision.wifi.subtitle");
  setTextById("provWifiSsidLabel", "provision.wifi.ssid");
  setTextById("provWifiCountryCodeLabel", "provision.wifi.country_code");
  setTextById("provWifiPasswordLabel", "provision.wifi.password");
  setTextById("provWifiShowPasswordLabel", "provision.wifi.show_password");
  setTextById("provScanWifiBtn", "common.scan");
  setTextById("provHaTitle", "provision.ha.title");
  setTextById("provHaSubtitle", "provision.ha.subtitle");
  setTextById("provHaUrlLabel", "provision.ha.ws_url");
  setTextById("provHaTokenLabel", "provision.ha.token");
  setTextById("provHaShowTokenLabel", "provision.ha.show_token");
  setTextById("provWifiSaveBtn", "common.save_reboot");
  setTextById("provHaSaveBtn", "common.save_reboot");

  setTextById("settingsWifiHeading", "settings.wifi.heading");
  setTextById("settingsWifiSsidLabel", "settings.wifi.ssid");
  setTextById("settingsWifiCountryCodeLabel", "settings.wifi.country_code");
  setTextById("settingsWifiBssidLabel", "settings.wifi.bssid");
  setTextById("settingsWifiPasswordLabel", "settings.wifi.password");
  setTextById("scanWifiBtn", "common.scan_wifi");

  setTextById("settingsHaHeading", "settings.ha.heading");
  setTextById("settingsHaUrlLabel", "settings.ha.ws_url");
  setTextById("settingsHaTokenLabel", "settings.ha.token");
  setTextById("settingsHaRestEnabledLabel", "settings.ha.rest_fallback");

  setTextById("settingsTimeHeading", "settings.time.heading");
  setTextById("settingsNtpServerLabel", "settings.time.ntp_server");
  setTextById("settingsTimezoneLabel", "settings.time.timezone");

  setTextById("settingsUiHeading", "settings.ui.heading");
  setTextById("settingsLanguageLabel", "settings.ui.language");
  setTextById("reloadLanguagesBtn", "settings.ui.reload_languages");
  setTextById("downloadLanguageBtn", "settings.ui.download_json");
  setTextById("uploadLanguageCodeLabel", "settings.ui.upload_code");
  setTextById("uploadLanguageFileLabel", "settings.ui.upload_file");
  setTextById("uploadLanguageBtn", "settings.ui.upload_button");

  setTextById("settingsApHeading", "settings.ap.heading");
  const apHint = document.getElementById("settingsApHint");
  if (apHint) apHint.innerHTML = t("settings.ap.hint");

  setTextById("settingsActionsHeading", "settings.actions.heading");
  setTextById("reloadSettingsBtn", "settings.actions.reload");
  setTextById("saveSettingsBtn", "settings.actions.save");
  setTextById("settingsActionsHint", "settings.actions.hint");

  if (el.settingsTranslationInfo && !el.settingsTranslationInfo.textContent) {
    el.settingsTranslationInfo.textContent = t("settings.translation.info");
  }

  if (el.uploadLanguageCode) {
    el.uploadLanguageCode.placeholder = "fr";
  }
}

function downloadLanguageJson() {
  const lang = normalizeUiLanguage(el.settingsLanguage?.value || editor.i18nLanguage);
  const web = {};
  for (const [key, value] of Object.entries(editor.i18nMap || {})) {
    web[key] = value;
  }

  const payload = {
    meta: {
      code: lang,
      exported_at: new Date().toISOString(),
    },
    web,
    lvgl: editor.i18nEffective?.lvgl || {},
  };

  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `betta-i18n-${lang}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

async function uploadLanguageJson() {
  const lang = normalizeLanguageCode(el.uploadLanguageCode?.value, "");
  if (!lang) {
    throw new Error(t("settings.translation.invalid_code"));
  }
  const file = el.uploadLanguageFile?.files?.[0];
  if (!file) {
    throw new Error(t("settings.translation.no_file"));
  }

  const text = await file.text();
  let parsed = null;
  try {
    parsed = JSON.parse(text);
  } catch (_) {
    throw new Error("Invalid JSON");
  }
  if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
    throw new Error("JSON must be an object");
  }

  const response = await fetch(`/api/i18n/custom?lang=${encodeURIComponent(lang)}`, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(parsed),
  });
  if (!response.ok) {
    let detail = await response.text();
    try {
      const json = JSON.parse(detail);
      detail = json.error || detail;
    } catch (_) {}
    throw new Error(detail);
  }
}

function setProvisioningVisible(visible) {
  if (el.provisioningRoot) {
    el.provisioningRoot.classList.toggle("hidden", !visible);
  }
  if (el.editorShell) {
    el.editorShell.classList.toggle("hidden", visible);
  }
}

function provisioningStageForSettings(settings) {
  const wifiConfigured = Boolean(settings?.wifi?.configured);
  const wifiSetupApActive = Boolean(settings?.wifi?.setup_ap_active);
  const haConfigured = Boolean(settings?.ha?.configured);
  if (wifiSetupApActive) return "wifi";
  if (!wifiConfigured) return "wifi";
  if (!haConfigured) return "ha";
  return null;
}

function showProvisioningStage(stage, settings) {
  if (!stage) {
    editor.provisioningStage = null;
    setProvisioningVisible(false);
    return false;
  }

  editor.provisioningStage = stage;
  const wifi = settings?.wifi || {};
  const ha = settings?.ha || {};
  editor.wifiScanSupported = wifi.scan_supported !== false;

  if (el.provisioningWifiPage) {
    el.provisioningWifiPage.classList.toggle("hidden", stage !== "wifi");
  }
  if (el.provisioningHaPage) {
    el.provisioningHaPage.classList.toggle("hidden", stage !== "ha");
  }
  setProvisioningVisible(true);

  if (el.provWifiSsid) {
    el.provWifiSsid.value = wifi.ssid || "";
  }
  if (el.provWifiCountryCode) {
    el.provWifiCountryCode.value = normalizeCountryCode(wifi.country_code) || "US";
  }
  if (el.provWifiPassword) {
    el.provWifiPassword.value = "";
    el.provWifiPassword.type = "password";
  }
  if (el.provWifiShowPassword) {
    el.provWifiShowPassword.checked = false;
  }
  if (el.provHaUrl) {
    el.provHaUrl.value = ha.ws_url || "";
  }
  if (el.provHaToken) {
    el.provHaToken.value = "";
    el.provHaToken.type = "password";
  }
  if (el.provHaShowToken) {
    el.provHaShowToken.checked = false;
  }

  if (el.provScanWifiBtn) {
    el.provScanWifiBtn.disabled = !editor.wifiScanSupported || editor.wifiScanInProgress;
  }
  renderWifiScanResults(editor.wifiScanItems, "provisioning");

  if (!editor.wifiScanSupported) {
    setWifiScanInfo(t("wifi.scan_unavailable"), false, "provisioning");
  } else if (!editor.wifiScanHasRun && !editor.wifiScanInProgress) {
    setWifiScanInfo(t("wifi.scan_click_short"), false, "provisioning");
  }

  setProvisioningInfo(
    stage,
    stage === "wifi"
      ? t("provision.wifi.hint", {}, "Save reboots the panel. After reboot, HA provisioning is shown.")
      : t("provision.ha.hint", {}, "Save reboots the panel. After reboot, the editor is unlocked.")
  );
  return true;
}

function setActivePane(pane) {
  editor.activePane = pane === "settings" ? "settings" : "layout";
  const showLayout = editor.activePane === "layout";
  el.layoutPane.classList.toggle("hidden", !showLayout);
  el.settingsPane.classList.toggle("hidden", showLayout);
  el.layoutTabBtn.classList.toggle("active", showLayout);
  el.settingsTabBtn.classList.toggle("active", !showLayout);
}

function setSectionCollapsed(sectionKey, collapsed) {
  const map = {
    pages: { section: el.pagesSection, toggle: el.togglePagesSection },
    widgets: { section: el.widgetsSection, toggle: el.toggleWidgetsSection },
    inspector: { section: el.inspectorSection, toggle: el.toggleInspectorSection },
  };
  const entry = map[sectionKey];
  if (!entry || !entry.section || !entry.toggle) return;

  const nextCollapsed = Boolean(collapsed);
  editor.sectionCollapsed[sectionKey] = nextCollapsed;
  entry.section.classList.toggle("collapsed", nextCollapsed);
  entry.toggle.textContent = nextCollapsed ? "+" : "-";
  entry.toggle.setAttribute("aria-expanded", nextCollapsed ? "false" : "true");
}

function toggleSection(sectionKey) {
  setSectionCollapsed(sectionKey, !editor.sectionCollapsed[sectionKey]);
}

function applySectionCollapseState() {
  setSectionCollapsed("pages", editor.sectionCollapsed.pages);
  setSectionCollapsed("widgets", editor.sectionCollapsed.widgets);
  setSectionCollapsed("inspector", editor.sectionCollapsed.inspector);
}

function renderSettings() {
  const settings = editor.settings || {};
  const wifi = settings.wifi || {};
  const ha = settings.ha || {};
  const time = settings.time || {};
  const ui = settings.ui || {};
  const scanSupported = wifi.scan_supported !== false;
  editor.wifiScanSupported = scanSupported;

  el.settingsWifiSsid.value = wifi.ssid || "";
  if (el.settingsWifiCountryCode) {
    el.settingsWifiCountryCode.value = normalizeCountryCode(wifi.country_code) || "US";
  }
  if (el.settingsWifiBssid) {
    el.settingsWifiBssid.value = normalizeBssid(wifi.bssid || "");
  }
  el.settingsWifiPassword.value = "";
  el.settingsHaUrl.value = ha.ws_url || "";
  el.settingsHaToken.value = "";
  if (el.settingsHaRestEnabled) {
    el.settingsHaRestEnabled.checked = ha.rest_enabled === true;
  }
  el.settingsNtpServer.value = time.ntp_server || "";
  el.settingsTimezone.value = time.timezone || "";
  if (el.settingsLanguage) {
    el.settingsLanguage.value = normalizeUiLanguage(ui.language);
  }
  renderLanguageOptions();

  const connectedRssiText = Number.isFinite(Number(wifi.rssi_dbm))
    ? `${Math.round(Number(wifi.rssi_dbm))} dBm`
    : "n/a";
  const connectedBssid = normalizeBssid(wifi.connected_bssid || "");
  const connectedChannel = Number.isFinite(Number(wifi.connected_channel))
    ? String(Math.round(Number(wifi.connected_channel)))
    : "n/a";

  el.settingsWifiInfo.textContent = [
    `${t("settings.info.configured")}: ${wifi.configured ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.connected")}: ${wifi.connected ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.password_stored")}: ${wifi.password_set ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.country")}: ${normalizeCountryCode(wifi.country_code) || "US"}`,
    `${t("settings.info.rssi")}: ${connectedRssiText}`,
    `${t("settings.info.connected_bssid")}: ${connectedBssid || "n/a"}`,
    `${t("settings.info.channel")}: ${connectedChannel}`,
  ].join(" | ");

  el.settingsHaInfo.textContent = [
    `${t("settings.info.configured")}: ${ha.configured ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.connected")}: ${ha.connected ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.token_stored")}: ${ha.access_token_set ? t("common.yes") : t("common.no")}`,
    `${t("settings.info.rest_fallback")}: ${ha.rest_enabled ? t("common.yes") : t("common.no")}`,
  ].join(" | ");

  el.settingsTimeInfo.textContent = t("settings.time.info");
  if (el.settingsUiInfo) {
    el.settingsUiInfo.textContent = t("settings.ui.info");
  }
  if (el.settingsTranslationInfo && !el.settingsTranslationInfo.textContent) {
    el.settingsTranslationInfo.textContent = t("settings.translation.info");
  }

  if (wifi.setup_ap_active) {
    const ssid = wifi.setup_ap_ssid || "(unknown)";
    el.settingsApInfo.textContent = t("settings.ap.active", { ssid });
  } else {
    el.settingsApInfo.textContent = t("settings.ap.inactive");
  }

  if (!scanSupported) {
    setWifiScanInfo(t("wifi.scan_unavailable"));
  } else if (!editor.wifiScanHasRun && !editor.wifiScanInProgress) {
    setWifiScanInfo(t("wifi.scan_click"));
  }
  if (el.scanWifiBtn) {
    el.scanWifiBtn.disabled = !scanSupported || editor.wifiScanInProgress;
  }
  renderWifiScanResults(editor.wifiScanItems);
}

function renderWifiScanResults(items, scope = "settings") {
  const ui = getWifiScanUi(scope);
  const select = ui.resultsSelect;
  if (!select) return;

  const currentSsid = ui.ssidInput ? ui.ssidInput.value.trim() : "";
  select.innerHTML = "";
  if (!editor.wifiScanSupported) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = t("wifi.scan.option_unavailable", {}, "Scan unavailable");
    select.appendChild(option);
    return;
  }

  if (editor.wifiScanInProgress) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = t("wifi.scan.option_scanning", {}, "Scanning...");
    select.appendChild(option);
    return;
  }

  if (!editor.wifiScanHasRun) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = t("wifi.scan.option_not_run", {}, "No scan yet");
    select.appendChild(option);
    return;
  }

  if (!Array.isArray(items) || items.length === 0) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = t("wifi.scan.option_no_networks", {}, "No networks found");
    select.appendChild(option);
    return;
  }

  const placeholder = document.createElement("option");
  placeholder.value = "";
  placeholder.textContent = t("wifi.scan.option_select", { count: items.length }, `Select network (${items.length} found)`);
  select.appendChild(placeholder);

  let firstMatchingSsidValue = "";
  for (let idx = 0; idx < items.length; idx++) {
    const net = items[idx];
    if (!net || typeof net.ssid !== "string" || !net.ssid.length) continue;

    const bssid = normalizeBssid(net.bssid || "");
    const rssiText = Number.isFinite(Number(net.rssi)) ? `${Math.round(Number(net.rssi))} dBm` : "n/a";
    const authmodeText = (typeof net.authmode === "string" && net.authmode.length) ? net.authmode : "unknown";
    const channelText = Number.isFinite(Number(net.channel)) ? `ch ${Math.round(Number(net.channel))}` : "ch ?";
    const details = [rssiText, authmodeText, channelText];
    if (bssid) {
      details.push(bssid);
    }
    if (net.connected === true) {
      details.push(t("wifi.scan.connected_tag", {}, "connected"));
    }

    const option = document.createElement("option");
    option.value = bssid || `${net.ssid}#${idx}`;
    option.dataset.ssid = net.ssid;
    option.dataset.bssid = bssid;
    option.textContent = `${net.ssid} (${details.join(", ")})`;
    select.appendChild(option);

    if (!firstMatchingSsidValue && currentSsid && net.ssid === currentSsid) {
      firstMatchingSsidValue = option.value;
    }
  }

  if (firstMatchingSsidValue) {
    select.value = firstMatchingSsidValue;
  }
}

async function scanWifiNetworks(scope = "settings") {
  const ui = getWifiScanUi(scope);
  if (!ui.scanButton) return;
  if (!editor.wifiScanSupported) {
    setWifiScanInfo(
      t("wifi.scan_unavailable"),
      false,
      scope
    );
    return;
  }
  if (editor.wifiScanInProgress) return;

  editor.wifiScanInProgress = true;
  ui.scanButton.disabled = true;
  renderWifiScanResults([], scope);
  setWifiScanInfo(t("status.wifi_scan_running"), false, scope);
  setStatus(t("status.wifi_scan_running"));

  const controller = new AbortController();
  const timeoutId = window.setTimeout(() => controller.abort(), 15000);
  try {
    const response = await fetch("/api/wifi/scan", {
      cache: "no-store",
      signal: controller.signal,
    });
    const body = await response.text();
    let data = null;
    if (body) {
      try {
        data = JSON.parse(body);
      } catch (_) {
        data = null;
      }
    }

    if (!response.ok) {
      const detail = data?.message || data?.error || `${response.status} ${response.statusText}`;
      throw new Error(detail);
    }

    data = data || {};
    editor.wifiScanItems = Array.isArray(data.items) ? data.items : [];
    editor.wifiScanHasRun = true;
    renderWifiScanResults(editor.wifiScanItems, scope);

    if (editor.wifiScanItems.length > 0) {
      setWifiScanInfo(
        t("wifi.scan_found", { count: editor.wifiScanItems.length }),
        false,
        scope
      );
    } else {
      setWifiScanInfo(t("wifi.scan_no_networks"), false, scope);
    }
    setStatus(t("status.wifi_scan_complete", { count: editor.wifiScanItems.length }));
  } catch (err) {
    editor.wifiScanHasRun = true;
    renderWifiScanResults(editor.wifiScanItems, scope);
    const detail = err?.name === "AbortError" ? "Wi-Fi scan request timed out" : (err?.message || "Unknown error");
    setWifiScanInfo(detail, true, scope);
    setStatus(t("status.wifi_scan_failed", { error: detail }), true);
  } finally {
    window.clearTimeout(timeoutId);
    editor.wifiScanInProgress = false;
    renderWifiScanResults(editor.wifiScanItems, scope);
    ui.scanButton.disabled = false;
  }
}

async function loadSettings(silent = false) {
  if (!silent) {
    setStatus(t("status.loading_settings"));
  }
  try {
    editor.settings = await apiGet("/api/settings");
    await loadI18nLanguage(editor.settings?.ui?.language || DEFAULT_UI_LANGUAGE, true);
    renderSettings();
    if (!silent) {
      setStatus(t("status.settings_loaded"));
    }
    return editor.settings;
  } catch (err) {
    if (!silent) {
      setStatus(t("status.settings_load_failed", { error: err.message }), true);
    }
    return null;
  }
}

async function putSettings(payload) {
  const response = await fetch("/api/settings", {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  if (!response.ok) {
    let detail = await response.text();
    try {
      const json = JSON.parse(detail);
      detail = json.error || detail;
    } catch (_) {}
    throw new Error(detail);
  }
}

async function saveWifiProvisioning() {
  const ssid = el.provWifiSsid?.value.trim() || "";
  const password = el.provWifiPassword?.value || "";
  const countryCode = normalizeCountryCode(el.provWifiCountryCode?.value) || "";

  if (!ssid) {
    setProvisioningInfo("wifi", t("provision.wifi.required_ssid"), true);
    return;
  }
  if (!countryCode) {
    setProvisioningInfo("wifi", t("provision.wifi.required_country"), true);
    return;
  }

  const payload = {
    wifi: {
      ssid,
      country_code: countryCode,
      bssid: null,
    },
    reboot: true,
  };
  if (password.length > 0) {
    payload.wifi.password = password;
  }

  setProvisioningInfo("wifi", t("provision.saving_reboot"));
  await putSettings(payload);
  setProvisioningInfo("wifi", t("provision.saved_reboot"));
}

async function saveHaProvisioning() {
  const wsUrl = el.provHaUrl?.value.trim() || "";
  const accessToken = el.provHaToken?.value.trim() || "";

  if (!wsUrl) {
    setProvisioningInfo("ha", t("provision.ha.required_url"), true);
    return;
  }
  if (!wsUrl.startsWith("ws://") && !wsUrl.startsWith("wss://")) {
    setProvisioningInfo("ha", t("provision.ha.invalid_url"), true);
    return;
  }
  if (!accessToken) {
    setProvisioningInfo("ha", t("provision.ha.required_token"), true);
    return;
  }

  const payload = {
    ha: {
      ws_url: wsUrl,
      access_token: accessToken,
    },
    reboot: true,
  };

  setProvisioningInfo("ha", t("provision.saving_reboot"));
  await putSettings(payload);
  setProvisioningInfo("ha", t("provision.saved_reboot"));
}

async function saveSettings() {
  const wifiSsid = el.settingsWifiSsid.value.trim();
  const wifiPassword = el.settingsWifiPassword.value;
  const wifiCountryCode = normalizeCountryCode(el.settingsWifiCountryCode?.value) || "";
  const wifiBssidRaw = el.settingsWifiBssid?.value || "";
  const wifiBssid = normalizeBssid(wifiBssidRaw);
  const haUrl = el.settingsHaUrl.value.trim();
  const haToken = el.settingsHaToken.value.trim();
  const haRestEnabled = Boolean(el.settingsHaRestEnabled?.checked);
  const ntpServer = el.settingsNtpServer.value.trim();
  const timezone = el.settingsTimezone.value.trim();
  const language = normalizeUiLanguage(el.settingsLanguage?.value);

  if (!wifiCountryCode) {
    setStatus(t("settings.language.invalid_country"), true);
    return;
  }
  if (wifiBssidRaw.trim().length > 0 && !wifiBssid) {
    setStatus(t("settings.language.invalid_bssid"), true);
    return;
  }
  if (haUrl && !haUrl.startsWith("ws://") && !haUrl.startsWith("wss://")) {
    setStatus(t("settings.language.invalid_ha_url"), true);
    return;
  }

  const payload = {
    wifi: {
      ssid: wifiSsid,
      country_code: wifiCountryCode,
      bssid: wifiBssid || null,
    },
    ha: {
      ws_url: haUrl,
      rest_enabled: haRestEnabled,
    },
    time: {
      ntp_server: ntpServer,
      timezone,
    },
    ui: {
      language,
    },
    reboot: true,
  };
  if (wifiPassword.length > 0) {
    payload.wifi.password = wifiPassword;
  }
  if (haToken.length > 0) {
    payload.ha.access_token = haToken;
  }

  setStatus(t("status.saving_settings"));
  await putSettings(payload);
  setStatus(t("status.settings_saved_reboot"));
}

function defaultLayout() {
  return {
    version: 1,
    pages: [
      {
        id: "wohnen",
        title: "Wohnen",
        widgets: [],
      },
    ],
  };
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function snap(value) {
  return Math.round(value / GRID) * GRID;
}

function selectedPage() {
  if (!editor.layout) return null;
  return editor.layout.pages.find((p) => p.id === editor.selectedPageId) || null;
}

function selectedWidget() {
  const page = selectedPage();
  if (!page) return null;
  return page.widgets.find((w) => w.id === editor.selectedWidgetId) || null;
}

function inspectorWidgetType() {
  return selectedWidget()?.type || el.fType?.value || "sensor";
}

function inspectorSliderEntityDomain() {
  const widget = selectedWidget();
  if (widget?.type === "slider") {
    return normalizeSliderEntityDomain(widget.slider_entity_domain);
  }
  return normalizeSliderEntityDomain(el.fSliderEntityDomain?.value);
}

function inspectorButtonMode() {
  const widget = selectedWidget();
  if (widget?.type === "button") {
    return normalizeButtonMode(widget.button_mode);
  }
  return normalizeButtonMode(el.fButtonMode?.value);
}

function allowedEntityDomainsForWidgetType(
  type,
  sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN,
  buttonMode = DEFAULT_BUTTON_MODE,
) {
  if (type === "empty_tile") return [];
  if (type === "sensor") return ["sensor"];
  if (type === "button") {
    const normalizedMode = normalizeButtonMode(buttonMode);
    return buttonModeRequiresMediaPlayer(normalizedMode) ? ["media_player"] : ["switch", "media_player"];
  }
  if (type === "light_tile") return ["light"];
  if (type === "heating_tile") return ["climate"];
  if (type === "weather_tile" || type === "weather_3day") return ["weather"];
  if (type === "slider") {
    const normalized = normalizeSliderEntityDomain(sliderDomain);
    if (normalized === "auto") {
      return ["light", "media_player", "cover"];
    }
    return [normalized];
  }
  return [];
}

function expectedDomainForWidgetType(
  type,
  sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN,
  buttonMode = DEFAULT_BUTTON_MODE,
) {
  const domains = allowedEntityDomainsForWidgetType(type, sliderDomain, buttonMode);
  return domains.length === 1 ? domains[0] : "";
}

function listEntitiesByDomain(domain) {
  if (!domain) return editor.entities;
  return editor.entities.filter((entity) => typeof entity.id === "string" && entity.id.startsWith(`${domain}.`));
}

function entityMatchesWidgetType(
  entity,
  type,
  sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN,
  buttonMode = DEFAULT_BUTTON_MODE,
) {
  if (type === "empty_tile") return true;

  const id = typeof entity?.id === "string" ? entity.id : "";
  if (!id) return false;

  const allowedDomains = allowedEntityDomainsForWidgetType(type, sliderDomain, buttonMode);
  if (!allowedDomains.length) return true;
  return allowedDomains.some((domain) => id.startsWith(`${domain}.`));
}

function listEntitiesForWidgetType(
  type,
  sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN,
  buttonMode = DEFAULT_BUTTON_MODE,
) {
  if (type === "empty_tile") return [];
  return editor.entities.filter((entity) => entityMatchesWidgetType(entity, type, sliderDomain, buttonMode));
}

function pickDefaultEntityForWidgetType(
  type,
  sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN,
  buttonMode = DEFAULT_BUTTON_MODE,
) {
  if (type === "empty_tile") return "";
  const matching = listEntitiesForWidgetType(type, sliderDomain, buttonMode);
  if (matching.length > 0) return matching[0].id;
  return editor.entities[0]?.id || "sensor.example";
}

function uniqueId(prefix, list, accessor = (x) => x.id) {
  let i = 1;
  while (true) {
    const candidate = `${prefix}_${i}`;
    if (!list.some((entry) => accessor(entry) === candidate)) return candidate;
    i += 1;
  }
}

function sanitizeIdPart(value) {
  const normalized = String(value || "")
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9_]+/g, "_")
    .replace(/^_+|_+$/g, "");
  return normalized || "item";
}

function createWidgetIdForPage(page, type) {
  const pagePart = sanitizeIdPart(page?.id || page?.title || "page");
  const typePart = sanitizeIdPart(type || "widget");
  const allWidgets = editor.layout?.pages?.flatMap((p) => (Array.isArray(p.widgets) ? p.widgets : [])) || [];

  let i = 1;
  while (true) {
    const candidate = `${pagePart}_${typePart}_${i}`;
    if (!allWidgets.some((widget) => widget?.id === candidate)) {
      return candidate;
    }
    i += 1;
  }
}

async function apiGet(path) {
  const response = await fetch(path, { cache: "no-store" });
  if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);
  return response.json();
}

function setEntityOptionsList(target, entities) {
  target.innerHTML = "";
  for (const entity of entities) {
    if (!entity || typeof entity.id !== "string" || !entity.id.length) continue;
    const option = document.createElement("option");
    option.value = entity.id;
    option.label = `${entity.id} (${entity.name || entity.id})`;
    target.appendChild(option);
  }
}

function normalizeEntitySearchTerm(value) {
  if (typeof value !== "string") return "";
  const trimmed = value.trim();
  if (!trimmed) return "";
  const wildcardIndex = trimmed.indexOf("*");
  if (wildcardIndex < 0) return trimmed;
  return trimmed.slice(wildcardIndex + 1).trim();
}

function parseEntitySearchInput(rawValue, fallbackDomain = "") {
  const value = typeof rawValue === "string" ? rawValue.trim() : "";
  let domain = typeof fallbackDomain === "string" ? fallbackDomain.trim().toLowerCase() : "";
  let search = value;

  const dotIndex = value.indexOf(".");
  if (dotIndex > 0) {
    const candidateDomain = value.slice(0, dotIndex).trim().toLowerCase();
    if (/^[a-z0-9_]+$/.test(candidateDomain)) {
      domain = candidateDomain;
      search = value.slice(dotIndex + 1);
    }
  }

  search = normalizeEntitySearchTerm(search);
  return { domain, search };
}

function entityContainsSearch(entity, search) {
  if (!search) return true;
  const needle = search.toLowerCase();
  const id = String(entity?.id || "").toLowerCase();
  const name = String(entity?.name || "").toLowerCase();
  return id.includes(needle) || name.includes(needle);
}

function filterLocalEntitySuggestions(source, domain, search, maxItems) {
  const results = [];
  for (const entity of source) {
    if (!entity || typeof entity.id !== "string" || entity.id.length === 0) continue;
    if (domain && !entity.id.startsWith(`${domain}.`)) continue;
    if (!entityContainsSearch(entity, search)) continue;
    results.push(entity);
    if (results.length >= maxItems) break;
  }
  return results;
}

async function fetchEntitySuggestions(domain, search, limit) {
  const params = new URLSearchParams();
  if (domain) params.set("domain", domain);
  if (search) params.set("search", search);
  params.set("limit", String(limit));
  const data = await apiGet(`/api/entities?${params.toString()}`);
  return Array.isArray(data.items) ? data.items : [];
}

function primaryEntitySource() {
  const inspectorType = inspectorWidgetType();
  const sliderDomain = inspectorSliderEntityDomain();
  const buttonMode = inspectorButtonMode();
  const typedOptions = listEntitiesForWidgetType(inspectorType, sliderDomain, buttonMode);
  return typedOptions.length > 0 ? typedOptions : editor.entities;
}

function defaultPrimaryEntityDomain() {
  return expectedDomainForWidgetType(inspectorWidgetType(), inspectorSliderEntityDomain(), inspectorButtonMode());
}

function scheduleEntityAutocomplete(kind, immediate = false) {
  const isSecondary = kind === "secondary";
  const state = isSecondary ? entityAutocomplete.secondary : entityAutocomplete.primary;
  const input = isSecondary ? el.fSecondaryEntity : el.fEntity;
  const options = isSecondary ? el.sensorEntityOptions : el.entityOptions;
  if (!input || !options) return;
  if (!isSecondary && input.disabled) return;
  if (isSecondary && input.disabled) return;

  if (state.timerId !== null) {
    window.clearTimeout(state.timerId);
    state.timerId = null;
  }

  const run = async () => {
    const requestSeq = ++state.requestSeq;
    const raw = input.value || "";
    const fallbackDomain = isSecondary ? "sensor" : defaultPrimaryEntityDomain();
    const { domain, search } = parseEntitySearchInput(raw, fallbackDomain);
    const source = isSecondary ? listEntitiesByDomain("sensor") : primaryEntitySource();
    const allowedDomains = isSecondary
      ? ["sensor"]
      : allowedEntityDomainsForWidgetType(inspectorWidgetType(), inspectorSliderEntityDomain(), inspectorButtonMode());

    if (domain && allowedDomains.length > 0 && !allowedDomains.includes(domain)) {
      setEntityOptionsList(options, []);
      return;
    }

    const localResults = filterLocalEntitySuggestions(source, domain, search, ENTITY_AUTOCOMPLETE_MAX_ITEMS);
    setEntityOptionsList(options, localResults);

    const shouldQueryApi = domain.length > 0 || search.length >= 2;
    if (!shouldQueryApi) return;

    try {
      const remoteResults = await fetchEntitySuggestions(domain, search, ENTITY_AUTOCOMPLETE_MAX_ITEMS);
      if (requestSeq !== state.requestSeq) return;
      if (remoteResults.length > 0) {
        setEntityOptionsList(options, remoteResults);
      }
    } catch (_) {
      // Keep local fallback options.
    }
  };

  if (immediate) {
    void run();
    return;
  }

  state.timerId = window.setTimeout(() => {
    state.timerId = null;
    void run();
  }, ENTITY_AUTOCOMPLETE_DEBOUNCE_MS);
}

async function loadLayout() {
  setStatus("Loading layout...");
  try {
    editor.layout = await apiGet("/api/layout");
    if (!editor.layout || !Array.isArray(editor.layout.pages)) {
      editor.layout = defaultLayout();
    }
  } catch (err) {
    editor.layout = defaultLayout();
    setStatus(`Layout load failed, using default: ${err.message}`, true);
  }

  if (!editor.layout.pages.length) {
    editor.layout.pages.push({ id: "wohnen", title: "Wohnen", widgets: [] });
  }
  normalizeLayoutWidgets(editor.layout);
  editor.selectedPageId = editor.layout.pages[0].id;
  editor.selectedWidgetId = null;
  renderAll();
  setStatus("Layout loaded");
}

async function loadEntities() {
  try {
    const data = await apiGet("/api/entities");
    editor.entities = Array.isArray(data.items) ? data.items : [];
    renderEntityOptions();
  } catch (err) {
    setStatus(`Entity fetch failed: ${err.message}`, true);
  }
}

async function refreshStates() {
  try {
    const data = await apiGet("/api/state");
    editor.states = new Map();
    if (Array.isArray(data.items)) {
      for (const item of data.items) {
        editor.states.set(item.entity_id, item.state);
      }
    }
    renderCanvas();
  } catch (_) {
    // Keep previous preview values.
  }
}

function renderEntityOptions() {
  const inspectorType = inspectorWidgetType();
  const sliderDomain = inspectorSliderEntityDomain();
  const buttonMode = inspectorButtonMode();
  const inspectorOptions = listEntitiesForWidgetType(inspectorType, sliderDomain, buttonMode);
  const primaryOptions = inspectorOptions.length > 0 ? inspectorOptions : editor.entities;
  setEntityOptionsList(el.entityOptions, primaryOptions.slice(0, ENTITY_AUTOCOMPLETE_MAX_ITEMS));

  setEntityOptionsList(
    el.sensorEntityOptions,
    listEntitiesByDomain("sensor").slice(0, ENTITY_AUTOCOMPLETE_MAX_ITEMS),
  );

  const secondaryEnabled = inspectorType === "heating_tile";
  if (el.fSecondaryEntityWrap) {
    el.fSecondaryEntityWrap.classList.toggle("hidden", !secondaryEnabled);
  }
  el.fSecondaryEntity.disabled = !secondaryEnabled;
  if (!secondaryEnabled) {
    el.fSecondaryEntity.value = "";
  }

  const primaryEnabled = inspectorType !== "empty_tile";
  if (el.fEntityWrap) {
    el.fEntityWrap.classList.toggle("hidden", !primaryEnabled);
  }
  if (el.fEntity) {
    el.fEntity.disabled = !primaryEnabled;
    if (!primaryEnabled) {
      el.fEntity.value = "";
    }
  }
}

function renderPages() {
  el.pagesList.innerHTML = "";
  for (const page of editor.layout.pages) {
    const li = document.createElement("li");
    li.className = `list-item ${page.id === editor.selectedPageId ? "active" : ""}`;
    li.textContent = `${page.title || page.id}  [${page.id}]`;
    li.onclick = () => {
      editor.selectedPageId = page.id;
      editor.selectedWidgetId = null;
      renderAll();
    };
    el.pagesList.appendChild(li);
  }
  renderPagesMini();
}

function renderPagesMini() {
  if (!el.pagesMiniList) return;
  el.pagesMiniList.innerHTML = "";
  for (const page of editor.layout.pages) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `mini-page-btn ${page.id === editor.selectedPageId ? "active" : ""}`;
    button.textContent = page.title || page.id;
    button.title = `${page.title || page.id} [${page.id}]`;
    button.onclick = () => {
      editor.selectedPageId = page.id;
      editor.selectedWidgetId = null;
      renderAll();
    };
    el.pagesMiniList.appendChild(button);
  }
}

function renderPageEditor() {
  const page = selectedPage();
  if (!page) {
    el.pageTitleInput.value = "";
    el.pageTitleInput.disabled = true;
    el.applyPageBtn.disabled = true;
    return;
  }
  el.pageTitleInput.disabled = false;
  el.applyPageBtn.disabled = false;
  el.pageTitleInput.value = page.title || page.id;
}

function renderWidgets() {
  const page = selectedPage();
  el.widgetsList.innerHTML = "";
  if (!page) return;

  for (const widget of page.widgets) {
    const li = document.createElement("li");
    li.className = `list-item ${widget.id === editor.selectedWidgetId ? "active" : ""}`;
    li.textContent = `${widget.type}  [${widget.id}]`;
    li.onclick = () => {
      editor.selectedWidgetId = widget.id;
      renderAll();
    };
    el.widgetsList.appendChild(li);
  }
}

function geometryStyle(node, rect) {
  node.style.left = `${rect.x}px`;
  node.style.top = `${rect.y}px`;
  node.style.width = `${rect.w}px`;
  node.style.height = `${rect.h}px`;
}

function selectWidgetLive(widgetId, selectedBox) {
  editor.selectedWidgetId = widgetId;
  document.querySelectorAll(".widget-box.selected").forEach((node) => node.classList.remove("selected"));
  if (selectedBox) {
    selectedBox.classList.add("selected");
  }
  renderWidgets();
  renderInspector();
}

function attachDragAndResize(box, widget) {
  const startMove = (mode, downEvent) => {
    downEvent.preventDefault();
    downEvent.stopPropagation();
    const startX = downEvent.clientX;
    const startY = downEvent.clientY;
    const startRect = { ...widget.rect };
    let moved = false;

    box.classList.add(mode === "drag" ? "dragging" : "resizing");

    const onMove = (moveEvent) => {
      const dx = moveEvent.clientX - startX;
      const dy = moveEvent.clientY - startY;
      const limits = widgetSizeLimits(widget.type);
      const maxW = Math.min(limits.maxW, CANVAS_WIDTH);
      const maxH = Math.min(limits.maxH, CANVAS_HEIGHT);
      const minW = Math.min(limits.minW, maxW);
      const minH = Math.min(limits.minH, maxH);
      let nextX = widget.rect.x;
      let nextY = widget.rect.y;
      let nextW = widget.rect.w;
      let nextH = widget.rect.h;

      if (mode === "drag") {
        nextX = clamp(snap(startRect.x + dx), 0, CANVAS_WIDTH - startRect.w);
        nextY = clamp(snap(startRect.y + dy), 0, CANVAS_HEIGHT - startRect.h);
      } else {
        nextW = clamp(snap(startRect.w + dx), minW, Math.min(maxW, CANVAS_WIDTH - startRect.x));
        nextH = clamp(snap(startRect.h + dy), minH, Math.min(maxH, CANVAS_HEIGHT - startRect.y));
      }

      if (nextX !== widget.rect.x || nextY !== widget.rect.y || nextW !== widget.rect.w || nextH !== widget.rect.h) {
        moved = true;
        widget.rect.x = nextX;
        widget.rect.y = nextY;
        widget.rect.w = nextW;
        widget.rect.h = nextH;
        geometryStyle(box, widget.rect);
        renderInspector();
      }
    };

    const onUp = () => {
      document.removeEventListener("mousemove", onMove);
      document.removeEventListener("mouseup", onUp);
      box.classList.remove("dragging");
      box.classList.remove("resizing");
      if (moved) {
        renderWidgets();
        renderInspector();
      }
    };

    document.addEventListener("mousemove", onMove);
    document.addEventListener("mouseup", onUp);
  };

  box.addEventListener("mousedown", (event) => {
    if (event.button !== 0) return;
    if (event.target.classList.contains("resize-handle")) return;
    selectWidgetLive(widget.id, box);
    startMove("drag", event);
  });

  const resizeHandle = box.querySelector(".resize-handle");
  resizeHandle.addEventListener("mousedown", (event) => {
    if (event.button !== 0) return;
    selectWidgetLive(widget.id, box);
    startMove("resize", event);
  });
}

function renderCanvas() {
  const page = selectedPage();
  el.canvas.innerHTML = "";
  if (!page) {
    el.canvasTitle.textContent = "Canvas";
    return;
  }

  el.canvasTitle.textContent = `Canvas: ${page.title || page.id}`;

  for (const widget of page.widgets) {
    const box = document.createElement("div");
    const isEmptyTile = widget.type === "empty_tile";
    const isMediaPlayerButton = widget.type === "button" && String(widget.entity_id || "").startsWith("media_player.");
    const previewTitle = (isMediaPlayerButton && !String(widget.title || "").trim()) ? "" : (widget.title || widget.id);
    box.className = `widget-box ${isEmptyTile ? "empty-tile" : ""} ${widget.id === editor.selectedWidgetId ? "selected" : ""}`;
    box.dataset.widgetId = widget.id;
    box.style.zIndex = isEmptyTile ? "1" : "10";
    const previewState = isEmptyTile ? "design" : (editor.states.get(widget.entity_id) || "unavailable");
    box.innerHTML = `
      <div class="w-type">${widget.type}</div>
      <div class="w-title">${previewTitle}</div>
      <div class="w-state">${previewState}</div>
      <div class="resize-handle"></div>
    `;
    geometryStyle(box, widget.rect);
    attachDragAndResize(box, widget);
    el.canvas.appendChild(box);
  }
}

function renderInspector() {
  const widget = selectedWidget();
  if (!widget) {
    el.fTitle.value = "";
    el.fType.value = "sensor";
    el.fEntity.value = "";
    el.fSecondaryEntity.value = "";
    el.fX.value = "";
    el.fY.value = "";
    el.fW.value = "";
    el.fH.value = "";
    if (el.buttonOptions) {
      el.buttonOptions.classList.add("hidden");
    }
    if (el.sliderOptions) {
      el.sliderOptions.classList.add("hidden");
    }
    if (el.graphOptions) {
      el.graphOptions.classList.add("hidden");
    }
    if (el.fSliderDirection) {
      el.fSliderDirection.value = DEFAULT_SLIDER_DIRECTION;
    }
    if (el.fSliderEntityDomain) {
      el.fSliderEntityDomain.value = DEFAULT_SLIDER_ENTITY_DOMAIN;
    }
    if (el.fButtonAccentColor) {
      el.fButtonAccentColor.value = DEFAULT_BUTTON_ACCENT_COLOR;
    }
    if (el.fButtonMode) {
      el.fButtonMode.value = DEFAULT_BUTTON_MODE;
    }
    if (el.fSliderAccentColor) {
      el.fSliderAccentColor.value = DEFAULT_SLIDER_ACCENT_COLOR;
    }
    if (el.fGraphLineColor) {
      el.fGraphLineColor.value = DEFAULT_GRAPH_LINE_COLOR;
    }
    if (el.fGraphTimeWindowMin) {
      el.fGraphTimeWindowMin.value = String(DEFAULT_GRAPH_TIME_WINDOW_MIN);
    }
    if (el.fGraphPointCount) {
      el.fGraphPointCount.value = "";
    }
    renderEntityOptions();
    return;
  }
  el.fTitle.value = widget.title || "";
  el.fType.value = widget.type;
  el.fEntity.value = widget.entity_id || "";
  el.fSecondaryEntity.value = widget.secondary_entity_id || "";
  el.fX.value = widget.rect.x;
  el.fY.value = widget.rect.y;
  el.fW.value = widget.rect.w;
  el.fH.value = widget.rect.h;

  const isButton = widget.type === "button";
  const isSlider = widget.type === "slider";
  const isGraph = widget.type === "graph";
  if (el.buttonOptions) {
    el.buttonOptions.classList.toggle("hidden", !isButton);
  }
  if (el.sliderOptions) {
    el.sliderOptions.classList.toggle("hidden", !isSlider);
  }
  if (el.graphOptions) {
    el.graphOptions.classList.toggle("hidden", !isGraph);
  }
  if (isButton) {
    const accent = normalizeHexColor(widget.button_accent_color, DEFAULT_BUTTON_ACCENT_COLOR);
    const buttonMode = normalizeButtonMode(widget.button_mode);
    widget.button_accent_color = accent;
    widget.button_mode = buttonMode;
    if (el.fButtonAccentColor) {
      el.fButtonAccentColor.value = accent;
    }
    if (el.fButtonMode) {
      el.fButtonMode.value = buttonMode;
    }
  } else {
    if (el.fButtonAccentColor) {
      el.fButtonAccentColor.value = DEFAULT_BUTTON_ACCENT_COLOR;
    }
    if (el.fButtonMode) {
      el.fButtonMode.value = DEFAULT_BUTTON_MODE;
    }
  }
  if (isSlider) {
    const sliderEntityDomain = normalizeSliderEntityDomain(widget.slider_entity_domain);
    const direction = normalizeSliderDirection(widget.slider_direction);
    const accent = normalizeHexColor(widget.slider_accent_color, DEFAULT_SLIDER_ACCENT_COLOR);
    widget.slider_entity_domain = sliderEntityDomain;
    widget.slider_direction = direction;
    widget.slider_accent_color = accent;
    if (el.fSliderEntityDomain) {
      el.fSliderEntityDomain.value = sliderEntityDomain;
    }
    if (el.fSliderDirection) {
      el.fSliderDirection.value = direction;
    }
    if (el.fSliderAccentColor) {
      el.fSliderAccentColor.value = accent;
    }
  } else {
    if (el.fSliderEntityDomain) {
      el.fSliderEntityDomain.value = DEFAULT_SLIDER_ENTITY_DOMAIN;
    }
    if (el.fSliderDirection) {
      el.fSliderDirection.value = DEFAULT_SLIDER_DIRECTION;
    }
    if (el.fSliderAccentColor) {
      el.fSliderAccentColor.value = DEFAULT_SLIDER_ACCENT_COLOR;
    }
  }
  if (isGraph) {
    const lineColor = normalizeHexColor(widget.graph_line_color, DEFAULT_GRAPH_LINE_COLOR);
    const timeWindowMin = normalizeGraphTimeWindowMin(widget.graph_time_window_min);
    const pointCount = normalizeGraphPointCount(widget.graph_point_count);
    widget.graph_line_color = lineColor;
    widget.graph_time_window_min = timeWindowMin;
    if (pointCount > 0) {
      widget.graph_point_count = pointCount;
    } else {
      delete widget.graph_point_count;
    }
    if (el.fGraphLineColor) {
      el.fGraphLineColor.value = lineColor;
    }
    if (el.fGraphTimeWindowMin) {
      el.fGraphTimeWindowMin.value = String(widget.graph_time_window_min);
    }
    if (el.fGraphPointCount) {
      el.fGraphPointCount.value = pointCount > 0 ? String(pointCount) : "";
    }
  } else {
    if (el.fGraphLineColor) {
      el.fGraphLineColor.value = DEFAULT_GRAPH_LINE_COLOR;
    }
    if (el.fGraphTimeWindowMin) {
      el.fGraphTimeWindowMin.value = String(DEFAULT_GRAPH_TIME_WINDOW_MIN);
    }
    if (el.fGraphPointCount) {
      el.fGraphPointCount.value = "";
    }
  }

  renderEntityOptions();
}

function renderAll() {
  normalizeLayoutWidgets(editor.layout);
  renderPages();
  renderPageEditor();
  renderWidgets();
  renderInspector();
  renderCanvas();
}

function addPage() {
  const pageId = uniqueId("page", editor.layout.pages);
  editor.layout.pages.push({
    id: pageId,
    title: `Page ${editor.layout.pages.length + 1}`,
    widgets: [],
  });
  editor.selectedPageId = pageId;
  editor.selectedWidgetId = null;
  renderAll();
}

function deletePage() {
  if (!editor.layout.pages.length || !editor.selectedPageId) return;
  if (editor.layout.pages.length === 1) {
    setStatus("At least one page is required", true);
    return;
  }
  editor.layout.pages = editor.layout.pages.filter((p) => p.id !== editor.selectedPageId);
  editor.selectedPageId = editor.layout.pages[0].id;
  editor.selectedWidgetId = null;
  renderAll();
}

function applyPageName() {
  const page = selectedPage();
  if (!page) return;
  const nextTitle = el.pageTitleInput.value.trim();
  page.title = nextTitle || page.id;
  renderAll();
}

function addWidget(type) {
  const page = selectedPage();
  if (!page) return;
  const sliderDomain = DEFAULT_SLIDER_ENTITY_DOMAIN;
  const id = createWidgetIdForPage(page, type);
  const entityId = pickDefaultEntityForWidgetType(type, sliderDomain);
  const secondaryEntityId = type === "heating_tile" ? pickDefaultEntityForWidgetType("sensor") : "";
  const defaultW =
    type === "weather_3day" ? 360
      : (type === "light_tile" || type === "heating_tile" || type === "weather_tile" || type === "empty_tile") ? 300
      : 220;
  const defaultH =
    type === "weather_3day" ? 260
      : (type === "light_tile" || type === "heating_tile" || type === "weather_tile" || type === "empty_tile") ? 260
      : 120;
  const rect = clampRectToCanvas({ x: 20, y: 20, w: defaultW, h: defaultH }, type);

  const widget = {
    id,
    type,
    title: id,
    entity_id: entityId,
    secondary_entity_id: secondaryEntityId,
    rect,
  };
  if (type === "slider") {
    widget.slider_entity_domain = DEFAULT_SLIDER_ENTITY_DOMAIN;
    widget.slider_direction = DEFAULT_SLIDER_DIRECTION;
    widget.slider_accent_color = DEFAULT_SLIDER_ACCENT_COLOR;
  }
  if (type === "button") {
    widget.button_mode = DEFAULT_BUTTON_MODE;
    widget.button_accent_color = DEFAULT_BUTTON_ACCENT_COLOR;
  }
  if (type === "graph") {
    widget.graph_line_color = DEFAULT_GRAPH_LINE_COLOR;
    widget.graph_time_window_min = DEFAULT_GRAPH_TIME_WINDOW_MIN;
  }

  page.widgets.push(widget);
  editor.selectedWidgetId = id;
  renderAll();
}

function deleteWidget() {
  const page = selectedPage();
  if (!page || !editor.selectedWidgetId) return;
  page.widgets = page.widgets.filter((w) => w.id !== editor.selectedWidgetId);
  editor.selectedWidgetId = null;
  renderAll();
}

function applyInspector() {
  const widget = selectedWidget();
  if (!widget) return;

  const widgetType = widget.type;
  const sliderDomain = widgetType === "slider"
    ? normalizeSliderEntityDomain(el.fSliderEntityDomain?.value)
    : DEFAULT_SLIDER_ENTITY_DOMAIN;
  const buttonMode = widgetType === "button"
    ? normalizeButtonMode(el.fButtonMode?.value)
    : DEFAULT_BUTTON_MODE;
  const nextEntityId = el.fEntity.value.trim() || pickDefaultEntityForWidgetType(widgetType, sliderDomain, buttonMode);

  if (!entityMatchesWidgetType({ id: nextEntityId }, widgetType, sliderDomain, buttonMode)) {
    const allowedDomains = allowedEntityDomainsForWidgetType(widgetType, sliderDomain, buttonMode);
    const allowedHint = allowedDomains.length ? allowedDomains.join(", ") : "the expected domain";
    setStatus(`Entity must use domain: ${allowedHint}`, true);
    return;
  }

  widget.title = el.fTitle.value.trim();
  widget.entity_id = nextEntityId;
  if (widgetType === "heating_tile") {
    const secondaryEntityId = el.fSecondaryEntity.value.trim() || pickDefaultEntityForWidgetType("sensor");
    if (!secondaryEntityId.startsWith("sensor.")) {
      setStatus("Ist Entity must start with sensor.", true);
      return;
    }
    widget.secondary_entity_id = secondaryEntityId;
  } else {
    widget.secondary_entity_id = "";
  }
  if (widgetType === "button") {
    widget.button_mode = buttonMode;
    widget.button_accent_color = normalizeHexColor(el.fButtonAccentColor?.value, DEFAULT_BUTTON_ACCENT_COLOR);
  } else {
    delete widget.button_mode;
    delete widget.button_accent_color;
  }
  if (widgetType === "slider") {
    widget.slider_entity_domain = sliderDomain;
    widget.slider_direction = normalizeSliderDirection(el.fSliderDirection?.value);
    widget.slider_accent_color = normalizeHexColor(el.fSliderAccentColor?.value, DEFAULT_SLIDER_ACCENT_COLOR);
  } else {
    delete widget.slider_entity_domain;
    delete widget.slider_direction;
    delete widget.slider_accent_color;
  }
  if (widgetType === "graph") {
    widget.graph_line_color = normalizeHexColor(el.fGraphLineColor?.value, DEFAULT_GRAPH_LINE_COLOR);
    widget.graph_time_window_min = normalizeGraphTimeWindowMin(el.fGraphTimeWindowMin?.value);
    const graphPointCount = normalizeGraphPointCount(el.fGraphPointCount?.value);
    if (graphPointCount > 0) {
      widget.graph_point_count = graphPointCount;
    } else {
      delete widget.graph_point_count;
    }
  } else {
    delete widget.graph_line_color;
    delete widget.graph_time_window_min;
    delete widget.graph_point_count;
  }
  widget.rect = clampRectToCanvas(
    {
      x: Number(el.fX.value || 0),
      y: Number(el.fY.value || 0),
      w: Number(el.fW.value || widget.rect.w),
      h: Number(el.fH.value || widget.rect.h),
    },
    widgetType
  );
  editor.selectedWidgetId = widget.id;
  renderAll();
}

async function saveLayout() {
  setStatus("Saving layout...");
  const response = await fetch("/api/layout", {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(editor.layout),
  });
  if (!response.ok) {
    let detail = await response.text();
    try {
      const json = JSON.parse(detail);
      detail = (json.errors || []).join(", ") || detail;
    } catch (_) {}
    throw new Error(detail);
  }
  setStatus("Layout saved");
}

function exportLayout() {
  const blob = new Blob([JSON.stringify(editor.layout, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "betta-layout.json";
  a.click();
  URL.revokeObjectURL(url);
}

function importLayoutFromText(text) {
  const parsed = JSON.parse(text);
  if (!parsed || !Array.isArray(parsed.pages)) {
    throw new Error("Invalid layout JSON");
  }
  normalizeLayoutWidgets(parsed);
  editor.layout = parsed;
  editor.selectedPageId = parsed.pages[0]?.id || null;
  editor.selectedWidgetId = null;
  renderAll();
  setStatus("Layout imported (not saved yet)");
}

function bindUi() {
  if (el.togglePagesSection) {
    el.togglePagesSection.onclick = () => toggleSection("pages");
  }
  if (el.toggleWidgetsSection) {
    el.toggleWidgetsSection.onclick = () => toggleSection("widgets");
  }
  if (el.toggleInspectorSection) {
    el.toggleInspectorSection.onclick = () => toggleSection("inspector");
  }
  applySectionCollapseState();

  el.layoutTabBtn.onclick = () => setActivePane("layout");
  el.settingsTabBtn.onclick = async () => {
    setActivePane("settings");
    await loadSettings(true);
  };
  el.addPageBtn.onclick = addPage;
  el.deletePageBtn.onclick = deletePage;
  el.applyPageBtn.onclick = applyPageName;
  el.addSensorBtn.onclick = () => addWidget("sensor");
  el.addButtonBtn.onclick = () => addWidget("button");
  el.addSliderBtn.onclick = () => addWidget("slider");
  el.addGraphBtn.onclick = () => addWidget("graph");
  el.addEmptyTileBtn.onclick = () => addWidget("empty_tile");
  el.addLightTileBtn.onclick = () => addWidget("light_tile");
  el.addHeatingTileBtn.onclick = () => addWidget("heating_tile");
  el.addWeatherTileBtn.onclick = () => addWidget("weather_tile");
  el.addWeather3DayBtn.onclick = () => addWidget("weather_3day");
  el.deleteWidgetBtn.onclick = deleteWidget;
  el.applyInspectorBtn.onclick = applyInspector;
  el.reloadBtn.onclick = () => loadLayout();
  el.fType.onchange = () => {
    const sliderDomain = normalizeSliderEntityDomain(el.fSliderEntityDomain?.value);
    const buttonMode = normalizeButtonMode(el.fButtonMode?.value);
    if (el.buttonOptions) {
      el.buttonOptions.classList.toggle("hidden", el.fType.value !== "button");
    }
    if (el.sliderOptions) {
      el.sliderOptions.classList.toggle("hidden", el.fType.value !== "slider");
    }
    if (el.graphOptions) {
      el.graphOptions.classList.toggle("hidden", el.fType.value !== "graph");
    }
    if (el.fType.value === "button") {
      if (el.fButtonMode) {
        el.fButtonMode.value = buttonMode;
      }
      if (el.fButtonAccentColor) {
        el.fButtonAccentColor.value = normalizeHexColor(el.fButtonAccentColor.value, DEFAULT_BUTTON_ACCENT_COLOR);
      }
    } else {
      if (el.fButtonMode) {
        el.fButtonMode.value = DEFAULT_BUTTON_MODE;
      }
      if (el.fButtonAccentColor) {
        el.fButtonAccentColor.value = DEFAULT_BUTTON_ACCENT_COLOR;
      }
    }
    if (el.fType.value === "slider") {
      if (el.fSliderEntityDomain) {
        el.fSliderEntityDomain.value = sliderDomain;
      }
      if (el.fSliderDirection) {
        el.fSliderDirection.value = normalizeSliderDirection(el.fSliderDirection.value);
      }
      if (el.fSliderAccentColor) {
        el.fSliderAccentColor.value = normalizeHexColor(el.fSliderAccentColor.value, DEFAULT_SLIDER_ACCENT_COLOR);
      }
    }
    if (el.fType.value === "graph") {
      if (el.fGraphLineColor) {
        el.fGraphLineColor.value = normalizeHexColor(el.fGraphLineColor.value, DEFAULT_GRAPH_LINE_COLOR);
      }
      if (el.fGraphTimeWindowMin) {
        el.fGraphTimeWindowMin.value = String(normalizeGraphTimeWindowMin(el.fGraphTimeWindowMin.value));
      }
      if (el.fGraphPointCount) {
        const normalizedGraphPoints = normalizeGraphPointCount(el.fGraphPointCount.value);
        el.fGraphPointCount.value = normalizedGraphPoints > 0 ? String(normalizedGraphPoints) : "";
      }
    } else {
      if (el.fGraphLineColor) {
        el.fGraphLineColor.value = DEFAULT_GRAPH_LINE_COLOR;
      }
      if (el.fGraphTimeWindowMin) {
        el.fGraphTimeWindowMin.value = String(DEFAULT_GRAPH_TIME_WINDOW_MIN);
      }
      if (el.fGraphPointCount) {
        el.fGraphPointCount.value = "";
      }
    }
    renderEntityOptions();
    const currentEntity = el.fEntity.value.trim();
    const effectiveButtonMode = el.fType.value === "button"
      ? normalizeButtonMode(el.fButtonMode?.value)
      : DEFAULT_BUTTON_MODE;
    if (!entityMatchesWidgetType({ id: currentEntity }, el.fType.value, sliderDomain, effectiveButtonMode)) {
      el.fEntity.value = pickDefaultEntityForWidgetType(el.fType.value, sliderDomain, effectiveButtonMode);
    }
    if (el.fType.value === "heating_tile") {
      const sensorEntity = el.fSecondaryEntity.value.trim();
      if (!sensorEntity.startsWith("sensor.")) {
        el.fSecondaryEntity.value = pickDefaultEntityForWidgetType("sensor");
      }
    } else {
      el.fSecondaryEntity.value = "";
    }
  };
  if (el.fEntity) {
    el.fEntity.oninput = () => scheduleEntityAutocomplete("primary");
    el.fEntity.onfocus = () => scheduleEntityAutocomplete("primary", true);
  }
  if (el.fButtonMode) {
    el.fButtonMode.onchange = () => {
      el.fButtonMode.value = normalizeButtonMode(el.fButtonMode.value);
      if (inspectorWidgetType() !== "button") return;
      renderEntityOptions();
      scheduleEntityAutocomplete("primary", true);
      const currentEntity = el.fEntity.value.trim();
      const buttonMode = inspectorButtonMode();
      if (!entityMatchesWidgetType({ id: currentEntity }, "button", DEFAULT_SLIDER_ENTITY_DOMAIN, buttonMode)) {
        el.fEntity.value = pickDefaultEntityForWidgetType("button", DEFAULT_SLIDER_ENTITY_DOMAIN, buttonMode);
      }
    };
  }
  if (el.fSliderEntityDomain) {
    el.fSliderEntityDomain.onchange = () => {
      el.fSliderEntityDomain.value = normalizeSliderEntityDomain(el.fSliderEntityDomain.value);
      if (inspectorWidgetType() !== "slider") return;
      scheduleEntityAutocomplete("primary", true);
      const currentEntity = el.fEntity.value.trim();
      const sliderDomain = inspectorSliderEntityDomain();
      if (!entityMatchesWidgetType({ id: currentEntity }, "slider", sliderDomain)) {
        el.fEntity.value = pickDefaultEntityForWidgetType("slider", sliderDomain);
      }
    };
  }
  if (el.fSecondaryEntity) {
    el.fSecondaryEntity.oninput = () => scheduleEntityAutocomplete("secondary");
    el.fSecondaryEntity.onfocus = () => scheduleEntityAutocomplete("secondary", true);
  }
  el.reloadSettingsBtn.onclick = () => loadSettings();
  el.scanWifiBtn.onclick = () => scanWifiNetworks("settings");
  el.settingsWifiScanResults.onchange = () => {
    const option = el.settingsWifiScanResults.selectedOptions?.[0];
    const ssid = option?.dataset?.ssid || "";
    if (ssid) {
      el.settingsWifiSsid.value = ssid;
    }
    const bssid = normalizeBssid(option?.dataset?.bssid || "");
    if (bssid && el.settingsWifiBssid) {
      el.settingsWifiBssid.value = bssid;
    }
  };
  if (el.settingsWifiCountryCode) {
    el.settingsWifiCountryCode.oninput = () => {
      const cleaned = (el.settingsWifiCountryCode.value || "")
        .toUpperCase()
        .replace(/[^A-Z]/g, "")
        .slice(0, 2);
      el.settingsWifiCountryCode.value = cleaned;
    };
  }
  if (el.settingsWifiBssid) {
    el.settingsWifiBssid.oninput = () => {
      const cleaned = (el.settingsWifiBssid.value || "")
        .toUpperCase()
        .replace(/[^0-9A-F:-]/g, "")
        .slice(0, 17);
      el.settingsWifiBssid.value = cleaned;
    };
  }
  if (el.uploadLanguageCode) {
    el.uploadLanguageCode.oninput = () => {
      const cleaned = (el.uploadLanguageCode.value || "")
        .toLowerCase()
        .replace(/[^a-z0-9_-]/g, "")
        .slice(0, 15);
      el.uploadLanguageCode.value = cleaned;
    };
  }
  if (el.settingsLanguage) {
    el.settingsLanguage.onchange = async () => {
      const lang = normalizeUiLanguage(el.settingsLanguage.value);
      await loadI18nLanguage(lang);
      if (el.uploadLanguageCode) {
        el.uploadLanguageCode.value = lang;
      }
      renderSettings();
    };
  }
  if (el.reloadLanguagesBtn) {
    el.reloadLanguagesBtn.onclick = async () => {
      try {
        await loadLanguageCatalog();
        renderLanguageOptions();
        if (el.settingsTranslationInfo) {
          el.settingsTranslationInfo.textContent = t("settings.translation.info");
        }
      } catch (err) {
        if (el.settingsTranslationInfo) {
          el.settingsTranslationInfo.textContent = t("settings.translation.upload_fail", { error: err.message });
          el.settingsTranslationInfo.classList.add("error");
        }
      }
    };
  }
  if (el.downloadLanguageBtn) {
    el.downloadLanguageBtn.onclick = async () => {
      const lang = normalizeUiLanguage(el.settingsLanguage?.value || editor.i18nLanguage);
      await loadI18nLanguage(lang);
      downloadLanguageJson();
    };
  }
  if (el.uploadLanguageBtn) {
    el.uploadLanguageBtn.onclick = async () => {
      if (el.settingsTranslationInfo) {
        el.settingsTranslationInfo.classList.remove("error");
      }
      try {
        const targetLang = normalizeLanguageCode(el.uploadLanguageCode?.value, "");
        await uploadLanguageJson();
        if (el.settingsTranslationInfo) {
          el.settingsTranslationInfo.textContent = t("settings.translation.upload_ok", { lang: targetLang });
          el.settingsTranslationInfo.classList.remove("error");
        }
        if (el.uploadLanguageFile) {
          el.uploadLanguageFile.value = "";
        }
        await loadI18nLanguage(targetLang || editor.i18nLanguage, true);
        renderSettings();
      } catch (err) {
        if (el.settingsTranslationInfo) {
          el.settingsTranslationInfo.textContent = t("settings.translation.upload_fail", { error: err.message });
          el.settingsTranslationInfo.classList.add("error");
        }
      }
    };
  }
  if (el.provScanWifiBtn) {
    el.provScanWifiBtn.onclick = () => scanWifiNetworks("provisioning");
  }
  if (el.provWifiScanResults) {
    el.provWifiScanResults.onchange = () => {
      const option = el.provWifiScanResults.selectedOptions?.[0];
      const ssid = option?.dataset?.ssid || "";
      if (ssid && el.provWifiSsid) {
        el.provWifiSsid.value = ssid;
      }
    };
  }
  if (el.provWifiCountryCode) {
    el.provWifiCountryCode.oninput = () => {
      const cleaned = (el.provWifiCountryCode.value || "")
        .toUpperCase()
        .replace(/[^A-Z]/g, "")
        .slice(0, 2);
      el.provWifiCountryCode.value = cleaned;
    };
  }
  if (el.provWifiShowPassword && el.provWifiPassword) {
    el.provWifiShowPassword.onchange = () => {
      el.provWifiPassword.type = el.provWifiShowPassword.checked ? "text" : "password";
    };
  }
  if (el.provHaShowToken && el.provHaToken) {
    el.provHaShowToken.onchange = () => {
      el.provHaToken.type = el.provHaShowToken.checked ? "text" : "password";
    };
  }
  if (el.provWifiSaveBtn) {
    el.provWifiSaveBtn.onclick = async () => {
      try {
        await saveWifiProvisioning();
      } catch (err) {
        setProvisioningInfo("wifi", `Save failed: ${err.message}`, true);
      }
    };
  }
  if (el.provHaSaveBtn) {
    el.provHaSaveBtn.onclick = async () => {
      try {
        await saveHaProvisioning();
      } catch (err) {
        setProvisioningInfo("ha", `Save failed: ${err.message}`, true);
      }
    };
  }
  el.saveSettingsBtn.onclick = async () => {
    try {
      await saveSettings();
    } catch (err) {
      setStatus(`Settings save failed: ${err.message}`, true);
    }
  };
  el.saveBtn.onclick = async () => {
    try {
      await saveLayout();
    } catch (err) {
      setStatus(`Save failed: ${err.message}`, true);
    }
  };
  el.exportBtn.onclick = exportLayout;
  el.importBtn.onclick = () => {
    const text = el.jsonPaste.value.trim();
    if (!text) return;
    try {
      importLayoutFromText(text);
    } catch (err) {
      setStatus(`Import failed: ${err.message}`, true);
    }
  };
  el.importFile.onchange = async (event) => {
    const file = event.target.files?.[0];
    if (!file) return;
    const text = await file.text();
    try {
      importLayoutFromText(text);
    } catch (err) {
      setStatus(`File import failed: ${err.message}`, true);
    }
  };
}

async function startEditor() {
  if (editor.editorStarted) return;
  editor.editorStarted = true;
  setProvisioningVisible(false);
  setActivePane("layout");
  await Promise.all([loadLayout(), loadEntities(), refreshStates()]);
  window.setInterval(refreshStates, 5000);
}

async function bootstrap() {
  bindUi();
  setStatus(t("status.idle"));
  const settings = await loadSettings(true);
  const stage = provisioningStageForSettings(settings);
  if (stage) {
    showProvisioningStage(stage, settings);
    return;
  }
  await startEditor();
}

bootstrap();
