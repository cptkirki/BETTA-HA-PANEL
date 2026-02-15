const GRID = 10;
const CANVAS_WIDTH = 720;
const CANVAS_HEIGHT = 600;
const MIN_WIDGET_SIZE = 60;
const DEFAULT_SLIDER_DIRECTION = "auto";
const DEFAULT_SLIDER_ACCENT_COLOR = "#6fe8ff";
const DEFAULT_GRAPH_LINE_COLOR = "#6fe8ff";
const DEFAULT_GRAPH_TIME_WINDOW_MIN = 120;
const GRAPH_POINTS_MIN = 16;
const GRAPH_POINTS_MAX = 64;
const GRAPH_TIME_WINDOW_MIN = 1;
const GRAPH_TIME_WINDOW_MAX = 1440;
const SLIDER_DIRECTIONS = new Set([
  "auto",
  "left_to_right",
  "right_to_left",
  "bottom_to_top",
  "top_to_bottom",
]);

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
      return { minW: 180, minH: 120, maxW: 480, maxH: 320 };
    case "slider":
      return { minW: MIN_WIDGET_SIZE, minH: MIN_WIDGET_SIZE, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "graph":
      return { minW: 220, minH: 140, maxW: CANVAS_WIDTH, maxH: CANVAS_HEIGHT };
    case "light_tile":
      return { minW: 200, minH: 180, maxW: 480, maxH: 480 };
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
  settings: null,
  wifiScanItems: [],
  wifiScanHasRun: false,
  wifiScanInProgress: false,
  wifiScanSupported: true,
  sectionCollapsed: {
    pages: false,
    widgets: false,
    inspector: false,
  },
};

const el = {
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
  fWidgetId: document.getElementById("fWidgetId"),
  fTitle: document.getElementById("fTitle"),
  fType: document.getElementById("fType"),
  fEntity: document.getElementById("fEntity"),
  fSecondaryEntityWrap: document.getElementById("fSecondaryEntityWrap"),
  fSecondaryEntity: document.getElementById("fSecondaryEntity"),
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
  scanWifiBtn: document.getElementById("scanWifiBtn"),
  settingsWifiScanResults: document.getElementById("settingsWifiScanResults"),
  settingsWifiScanInfo: document.getElementById("settingsWifiScanInfo"),
  settingsWifiPassword: document.getElementById("settingsWifiPassword"),
  settingsHaUrl: document.getElementById("settingsHaUrl"),
  settingsHaToken: document.getElementById("settingsHaToken"),
  settingsNtpServer: document.getElementById("settingsNtpServer"),
  settingsTimezone: document.getElementById("settingsTimezone"),
  settingsWifiInfo: document.getElementById("settingsWifiInfo"),
  settingsHaInfo: document.getElementById("settingsHaInfo"),
  settingsTimeInfo: document.getElementById("settingsTimeInfo"),
  settingsApInfo: document.getElementById("settingsApInfo"),
  reloadSettingsBtn: document.getElementById("reloadSettingsBtn"),
  saveSettingsBtn: document.getElementById("saveSettingsBtn"),
};

function normalizeSliderDirection(value) {
  return SLIDER_DIRECTIONS.has(value) ? value : DEFAULT_SLIDER_DIRECTION;
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
      if (widget.type === "slider") {
        widget.slider_direction = normalizeSliderDirection(widget.slider_direction);
        widget.slider_accent_color = normalizeHexColor(widget.slider_accent_color, DEFAULT_SLIDER_ACCENT_COLOR);
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

function setWifiScanInfo(text, isError = false) {
  if (!el.settingsWifiScanInfo) return;
  el.settingsWifiScanInfo.textContent = text;
  el.settingsWifiScanInfo.classList.toggle("error", isError);
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
  const scanSupported = wifi.scan_supported !== false;
  editor.wifiScanSupported = scanSupported;

  el.settingsWifiSsid.value = wifi.ssid || "";
  el.settingsWifiPassword.value = "";
  el.settingsHaUrl.value = ha.ws_url || "";
  el.settingsHaToken.value = "";
  el.settingsNtpServer.value = time.ntp_server || "";
  el.settingsTimezone.value = time.timezone || "";

  el.settingsWifiInfo.textContent = [
    `Configured: ${wifi.configured ? "yes" : "no"}`,
    `Connected: ${wifi.connected ? "yes" : "no"}`,
    `Password stored: ${wifi.password_set ? "yes" : "no"}`,
  ].join(" | ");

  el.settingsHaInfo.textContent = [
    `Configured: ${ha.configured ? "yes" : "no"}`,
    `Connected: ${ha.connected ? "yes" : "no"}`,
    `Token stored: ${ha.access_token_set ? "yes" : "no"}`,
  ].join(" | ");

  el.settingsTimeInfo.textContent = "Applied after reboot. Time sync starts when Wi-Fi is connected.";

  if (wifi.setup_ap_active) {
    const ssid = wifi.setup_ap_ssid || "(unknown)";
    el.settingsApInfo.textContent = `Setup AP active: ${ssid}\nOpen http://192.168.4.1 while connected to this AP.`;
  } else {
    el.settingsApInfo.textContent = "Setup AP inactive.\nUse the panel IP in your home Wi-Fi network.";
  }

  if (!scanSupported) {
    setWifiScanInfo("Wi-Fi scan is unavailable in setup AP mode on this hardware. Enter SSID manually.");
  } else if (!editor.wifiScanHasRun && !editor.wifiScanInProgress) {
    setWifiScanInfo('Click "Scan Wi-Fi" to list nearby networks.');
  }
  if (el.scanWifiBtn) {
    el.scanWifiBtn.disabled = !scanSupported || editor.wifiScanInProgress;
  }
  renderWifiScanResults(editor.wifiScanItems);
}

function renderWifiScanResults(items) {
  const select = el.settingsWifiScanResults;
  if (!select) return;

  const currentSsid = el.settingsWifiSsid.value.trim();
  select.innerHTML = "";
  if (!editor.wifiScanSupported) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "Scan unavailable";
    select.appendChild(option);
    return;
  }

  if (editor.wifiScanInProgress) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "Scanning...";
    select.appendChild(option);
    return;
  }

  if (!editor.wifiScanHasRun) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "No scan yet";
    select.appendChild(option);
    return;
  }

  if (!Array.isArray(items) || items.length === 0) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = "No networks found";
    select.appendChild(option);
    return;
  }

  const placeholder = document.createElement("option");
  placeholder.value = "";
  placeholder.textContent = `Select network (${items.length} found)`;
  select.appendChild(placeholder);

  for (const net of items) {
    if (!net || typeof net.ssid !== "string" || !net.ssid.length) continue;
    const option = document.createElement("option");
    option.value = net.ssid;
    option.textContent = `${net.ssid} (${net.rssi} dBm, ${net.authmode})`;
    select.appendChild(option);
  }

  if (currentSsid) {
    select.value = currentSsid;
  }
}

async function scanWifiNetworks() {
  if (!el.scanWifiBtn) return;
  if (!editor.wifiScanSupported) {
    setWifiScanInfo("Wi-Fi scan is unavailable in setup AP mode on this hardware. Enter SSID manually.");
    return;
  }
  if (editor.wifiScanInProgress) return;

  editor.wifiScanInProgress = true;
  el.scanWifiBtn.disabled = true;
  renderWifiScanResults([]);
  setWifiScanInfo("Scanning nearby networks (max 8s)...");
  setStatus("Scanning Wi-Fi networks...");

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
    renderWifiScanResults(editor.wifiScanItems);

    if (editor.wifiScanItems.length > 0) {
      setWifiScanInfo(`${editor.wifiScanItems.length} network(s) found. Select one to fill SSID.`);
    } else {
      setWifiScanInfo("No networks found. Move closer to your router and scan again.");
    }
    setStatus(`Wi-Fi scan complete (${editor.wifiScanItems.length} networks)`);
  } catch (err) {
    editor.wifiScanHasRun = true;
    renderWifiScanResults(editor.wifiScanItems);
    const detail = err?.name === "AbortError" ? "Wi-Fi scan request timed out" : (err?.message || "Unknown error");
    setWifiScanInfo(detail, true);
    setStatus(`Wi-Fi scan failed: ${detail}`, true);
  } finally {
    window.clearTimeout(timeoutId);
    editor.wifiScanInProgress = false;
    renderWifiScanResults(editor.wifiScanItems);
    el.scanWifiBtn.disabled = false;
  }
}

async function loadSettings(silent = false) {
  if (!silent) {
    setStatus("Loading settings...");
  }
  try {
    editor.settings = await apiGet("/api/settings");
    renderSettings();
    if (!silent) {
      setStatus("Settings loaded");
    }
  } catch (err) {
    if (!silent) {
      setStatus(`Settings load failed: ${err.message}`, true);
    }
  }
}

async function saveSettings() {
  const wifiSsid = el.settingsWifiSsid.value.trim();
  const wifiPassword = el.settingsWifiPassword.value;
  const haUrl = el.settingsHaUrl.value.trim();
  const haToken = el.settingsHaToken.value.trim();
  const ntpServer = el.settingsNtpServer.value.trim();
  const timezone = el.settingsTimezone.value.trim();

  if (haUrl && !haUrl.startsWith("ws://") && !haUrl.startsWith("wss://")) {
    setStatus("HA URL must start with ws:// or wss://", true);
    return;
  }

  const payload = {
    wifi: {
      ssid: wifiSsid,
    },
    ha: {
      ws_url: haUrl,
    },
    time: {
      ntp_server: ntpServer,
      timezone,
    },
    reboot: true,
  };
  if (wifiPassword.length > 0) {
    payload.wifi.password = wifiPassword;
  }
  if (haToken.length > 0) {
    payload.ha.access_token = haToken;
  }

  setStatus("Saving settings...");
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

  setStatus("Settings saved. Device reboots in ~2s. Reconnect and reopen the panel URL.");
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

function expectedDomainForWidgetType(type) {
  if (type === "button") return "switch";
  if (type === "light_tile") return "light";
  if (type === "heating_tile") return "climate";
  if (type === "weather_tile" || type === "weather_3day") return "weather";
  return "";
}

function listEntitiesByDomain(domain) {
  if (!domain) return editor.entities;
  return editor.entities.filter((entity) => typeof entity.id === "string" && entity.id.startsWith(`${domain}.`));
}

function entityMatchesWidgetType(entity, type) {
  const id = typeof entity?.id === "string" ? entity.id : "";
  if (!id) return false;

  if (type === "sensor") {
    return id.startsWith("sensor.") || id.startsWith("binary_sensor.");
  }

  const requiredDomain = expectedDomainForWidgetType(type);
  if (!requiredDomain) return true;
  return id.startsWith(`${requiredDomain}.`);
}

function listEntitiesForWidgetType(type) {
  return editor.entities.filter((entity) => entityMatchesWidgetType(entity, type));
}

function pickDefaultEntityForWidgetType(type) {
  const matching = listEntitiesForWidgetType(type);
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

async function apiGet(path) {
  const response = await fetch(path, { cache: "no-store" });
  if (!response.ok) throw new Error(`${response.status} ${response.statusText}`);
  return response.json();
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
  const inspectorType = selectedWidget()?.type || el.fType.value;
  const inspectorOptions = listEntitiesForWidgetType(inspectorType);
  el.entityOptions.innerHTML = "";
  for (const entity of inspectorOptions.length > 0 ? inspectorOptions : editor.entities) {
    const option = document.createElement("option");
    option.value = entity.id;
    option.label = `${entity.id} (${entity.name || entity.id})`;
    el.entityOptions.appendChild(option);
  }

  el.sensorEntityOptions.innerHTML = "";
  for (const entity of listEntitiesByDomain("sensor")) {
    const option = document.createElement("option");
    option.value = entity.id;
    option.label = `${entity.id} (${entity.name || entity.id})`;
    el.sensorEntityOptions.appendChild(option);
  }

  const secondaryEnabled = inspectorType === "heating_tile";
  if (el.fSecondaryEntityWrap) {
    el.fSecondaryEntityWrap.classList.toggle("hidden", !secondaryEnabled);
  }
  el.fSecondaryEntity.disabled = !secondaryEnabled;
  if (!secondaryEnabled) {
    el.fSecondaryEntity.value = "";
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
    box.className = `widget-box ${widget.id === editor.selectedWidgetId ? "selected" : ""}`;
    box.dataset.widgetId = widget.id;
    box.innerHTML = `
      <div class="w-type">${widget.type}</div>
      <div class="w-title">${widget.title || widget.id}</div>
      <div class="w-state">${editor.states.get(widget.entity_id) || "unavailable"}</div>
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
    el.fWidgetId.value = "";
    el.fTitle.value = "";
    el.fType.value = "sensor";
    el.fEntity.value = "";
    el.fSecondaryEntity.value = "";
    el.fX.value = "";
    el.fY.value = "";
    el.fW.value = "";
    el.fH.value = "";
    if (el.sliderOptions) {
      el.sliderOptions.classList.add("hidden");
    }
    if (el.graphOptions) {
      el.graphOptions.classList.add("hidden");
    }
    if (el.fSliderDirection) {
      el.fSliderDirection.value = DEFAULT_SLIDER_DIRECTION;
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
  el.fWidgetId.value = widget.id;
  el.fTitle.value = widget.title || "";
  el.fType.value = widget.type;
  el.fEntity.value = widget.entity_id || "";
  el.fSecondaryEntity.value = widget.secondary_entity_id || "";
  el.fX.value = widget.rect.x;
  el.fY.value = widget.rect.y;
  el.fW.value = widget.rect.w;
  el.fH.value = widget.rect.h;

  const isSlider = widget.type === "slider";
  const isGraph = widget.type === "graph";
  if (el.sliderOptions) {
    el.sliderOptions.classList.toggle("hidden", !isSlider);
  }
  if (el.graphOptions) {
    el.graphOptions.classList.toggle("hidden", !isGraph);
  }
  if (isSlider) {
    const direction = normalizeSliderDirection(widget.slider_direction);
    const accent = normalizeHexColor(widget.slider_accent_color, DEFAULT_SLIDER_ACCENT_COLOR);
    widget.slider_direction = direction;
    widget.slider_accent_color = accent;
    if (el.fSliderDirection) {
      el.fSliderDirection.value = direction;
    }
    if (el.fSliderAccentColor) {
      el.fSliderAccentColor.value = accent;
    }
  } else {
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
  const id = uniqueId(type, page.widgets);
  const entityId = pickDefaultEntityForWidgetType(type);
  const secondaryEntityId = type === "heating_tile" ? pickDefaultEntityForWidgetType("sensor") : "";
  const defaultW =
    type === "weather_3day" ? 360 : (type === "light_tile" || type === "heating_tile" || type === "weather_tile") ? 300 : 220;
  const defaultH =
    type === "weather_3day" ? 260 : (type === "light_tile" || type === "heating_tile" || type === "weather_tile") ? 260 : 120;
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
    widget.slider_direction = DEFAULT_SLIDER_DIRECTION;
    widget.slider_accent_color = DEFAULT_SLIDER_ACCENT_COLOR;
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
  const page = selectedPage();
  if (!widget || !page) return;

  const proposedId = el.fWidgetId.value.trim();
  if (proposedId && proposedId !== widget.id && page.widgets.some((w) => w.id === proposedId)) {
    setStatus("Widget ID already exists in this page", true);
    return;
  }

  widget.id = proposedId || widget.id;
  widget.title = el.fTitle.value.trim();
  widget.type = el.fType.value;
  widget.entity_id = el.fEntity.value.trim() || pickDefaultEntityForWidgetType(widget.type);
  if (widget.type === "heating_tile") {
    widget.secondary_entity_id = el.fSecondaryEntity.value.trim() || pickDefaultEntityForWidgetType("sensor");
  } else {
    widget.secondary_entity_id = "";
  }
  if (widget.type === "slider") {
    widget.slider_direction = normalizeSliderDirection(el.fSliderDirection?.value);
    widget.slider_accent_color = normalizeHexColor(el.fSliderAccentColor?.value, DEFAULT_SLIDER_ACCENT_COLOR);
  } else {
    delete widget.slider_direction;
    delete widget.slider_accent_color;
  }
  if (widget.type === "graph") {
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
    widget.type
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
  a.download = "smart86-layout.json";
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
  el.addLightTileBtn.onclick = () => addWidget("light_tile");
  el.addHeatingTileBtn.onclick = () => addWidget("heating_tile");
  el.addWeatherTileBtn.onclick = () => addWidget("weather_tile");
  el.addWeather3DayBtn.onclick = () => addWidget("weather_3day");
  el.deleteWidgetBtn.onclick = deleteWidget;
  el.applyInspectorBtn.onclick = applyInspector;
  el.reloadBtn.onclick = () => loadLayout();
  el.fType.onchange = () => {
    if (el.sliderOptions) {
      el.sliderOptions.classList.toggle("hidden", el.fType.value !== "slider");
    }
    if (el.graphOptions) {
      el.graphOptions.classList.toggle("hidden", el.fType.value !== "graph");
    }
    if (el.fType.value === "slider") {
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
    if (!entityMatchesWidgetType({ id: currentEntity }, el.fType.value)) {
      el.fEntity.value = pickDefaultEntityForWidgetType(el.fType.value);
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
  el.reloadSettingsBtn.onclick = () => loadSettings();
  el.scanWifiBtn.onclick = () => scanWifiNetworks();
  el.settingsWifiScanResults.onchange = () => {
    const ssid = el.settingsWifiScanResults.value;
    if (ssid) {
      el.settingsWifiSsid.value = ssid;
    }
  };
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

async function bootstrap() {
  bindUi();
  setActivePane("layout");
  await Promise.all([loadLayout(), loadEntities(), refreshStates(), loadSettings(true)]);
  window.setInterval(refreshStates, 5000);
}

bootstrap();
