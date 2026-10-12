
#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ---------- HARDWARE ----------
#define LED_PIN       5
#define NUM_LEDS      41
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB leds[NUM_LEDS];

// ---------- WI-FI ----------
const char* AP_SSID = "LED-Controller-41";
const char* AP_PASSWORD = "GlowControl42";

WebServer server(80);

// ---------- CONTROLLER STATE ----------
enum Effect {
  EFFECT_SOLID,
  EFFECT_RAINBOW,
  EFFECT_CHASE,
  EFFECT_BREATHE,
  EFFECT_OFF
};

Effect currentEffect = EFFECT_OFF;
CRGB selectedColor = CRGB::Red;

uint8_t brightnessPercent = 20;
uint8_t rainbowHue = 0;
uint8_t chasePosition = 0;

unsigned long previousFrame = 0;
const unsigned long frameInterval = 20;

// ---------- MOBILE DASHBOARD ----------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#10131b">
<title>Glow | LED Controller</title>

<style>
:root {
  color-scheme: dark;
  --bg: #10131b;
  --panel: #1b202c;
  --panel2: #242b3a;
  --text: #f4f6ff;
  --muted: #aab3c5;
  --accent: #8c9cff;
  --border: #343c4e;
  --success: #78e3ac;
  --danger: #ff9292;
}

* { box-sizing: border-box; }

body {
  margin: 0;
  padding: 22px 15px 36px;
  background:
    radial-gradient(ellipse at top, #252d48 0%, var(--bg) 55%);
  color: var(--text);
  font-family: system-ui, -apple-system, sans-serif;
}

main { max-width: 520px; margin: auto; }

header { margin-bottom: 22px; }

.eyebrow {
  color: var(--accent);
  font-size: 12px;
  letter-spacing: 2px;
  font-weight: 800;
}

h1 {
  margin: 8px 0;
  font-size: clamp(27px, 7vw, 36px);
  letter-spacing: -1px;
}

p { color: var(--muted); line-height: 1.5; }

.card {
  background: rgba(27, 32, 44, .96);
  border: 1px solid var(--border);
  border-radius: 20px;
  padding: 19px;
  margin: 15px 0;
  box-shadow: 0 8px 26px #00000018;
}

.card h2 { margin: 0 0 15px; font-size: 17px; }

.preview {
  height: 105px;
  border-radius: 15px;
  background: #ff0000;
  border: 1px solid #ffffff24;
  box-shadow: 0 0 24px #ff000044;
  transition: background .12s, box-shadow .12s;
}

.preview-caption {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
  margin: 13px 0;
}

#hexLabel {
  font-family: ui-monospace, monospace;
  font-size: 14px;
  font-weight: 700;
}

.pill {
  border: 1px solid var(--border);
  background: var(--panel2);
  color: var(--muted);
  padding: 5px 10px;
  border-radius: 30px;
  font-size: 12px;
}

label {
  display: block;
  margin: 14px 0 9px;
  font-weight: 650;
  font-size: 14px;
}

input[type="color"] {
  display: block;
  width: 100%;
  height: 50px;
  padding: 3px;
  border: 1px solid var(--border);
  border-radius: 12px;
  background: var(--panel2);
  cursor: pointer;
}

input[type="range"] {
  width: 100%;
  accent-color: var(--accent);
  cursor: pointer;
}

.range-label {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
}

#brightnessValue {
  color: var(--accent);
  font-size: 21px;
  font-weight: 800;
}

button {
  border: 1px solid var(--border);
  border-radius: 12px;
  padding: 13px 12px;
  color: var(--text);
  background: var(--panel2);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  touch-action: manipulation;
  transition: transform .12s, border-color .12s, background .12s;
}

button:active { transform: scale(.98); }

button:focus-visible, input:focus-visible, select:focus-visible {
  outline: 2px solid var(--accent);
  outline-offset: 3px;
}

.primary {
  background: var(--accent);
  color: #101426;
  border-color: transparent;
}

.full { width: 100%; margin-top: 12px; }

.effects {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.effect-button {
  text-align: left;
  min-height: 70px;
}

.effect-button span {
  display: block;
  margin-top: 5px;
  font-size: 12px;
  font-weight: 400;
  color: var(--muted);
}

.effect-button.active {
  border-color: var(--accent);
  background: #343d5c;
  box-shadow: inset 0 0 0 1px var(--accent);
}

.effect-button.active::after {
  content: " ✓ ACTIVE";
  display: block;
  margin-top: 7px;
  color: var(--success);
  font-size: 10px;
  font-weight: 800;
}

.status {
  display: flex;
  align-items: flex-start;
  gap: 10px;
}

.dot {
  width: 9px;
  height: 9px;
  margin-top: 6px;
  border-radius: 50%;
  background: var(--success);
  flex: 0 0 auto;
}

.dot.error { background: var(--danger); }

#statusTitle { font-weight: 750; }

#statusMessage {
  margin: 4px 0 0;
  font-size: 13px;
  overflow-wrap: anywhere;
}

.note {
  font-size: 12px;
  text-align: center;
  margin-top: 22px;
}

button:disabled { opacity: .65; cursor: wait; }

@media (max-width: 360px) {
  body { padding: 15px 10px 28px; }
  .card { padding: 15px; }
  .effects { grid-template-columns: 1fr; }
}
</style>
</head>

<body>
<main>
  <header>
    <div class="eyebrow">ESP32 · SMART LIGHTING</div>
    <h1>Glow Controller</h1>
    <p>Make your space yours. Control all 41 LEDs from your phone.</p>
  </header>

  <section class="card">
    <h2>Light preview</h2>
    <div id="preview" class="preview" role="img"
         aria-label="Selected LED color preview"></div>
    <div class="preview-caption">
      <span id="hexLabel">#FF0000</span>
      <span class="pill">41 LEDs</span>
    </div>
    <label for="color">Choose your color</label>
    <input id="color" type="color" value="#ff0000">
    <button id="applyColor" class="primary full">Apply color</button>
  </section>

  <section class="card">
    <div class="range-label">
      <h2>Brightness</h2>
      <span id="brightnessValue">20%</span>
    </div>
    <input id="brightness" type="range"
           min="1" max="100" value="20"
           aria-label="Brightness percentage">
    <div class="range-label">
      <span class="pill">Low</span>
      <span class="pill">High</span>
    </div>
    <p style="font-size:12px">
      The preview adjusts instantly. Hardware updates are sent after
      a short pause while you move the slider.
    </p>
  </section>

  <section class="card">
    <h2>Lighting effects</h2>
    <p>Choose an effect. The active mode is highlighted.</p>
    <div class="effects">
      <button class="effect-button" data-mode="solid">
        Solid color <span>Your selected color</span>
      </button>
      <button class="effect-button" data-mode="rainbow">
        Rainbow <span>Flowing colors</span>
      </button>
      <button class="effect-button" data-mode="chase">
        Color chase <span>Moving light</span>
      </button>
      <button class="effect-button" data-mode="breathe">
        Breathe <span>Gentle pulsing</span>
      </button>
      <button class="effect-button" data-mode="off">
        Off <span>Turn all LEDs off</span>
      </button>
    </div>
  </section>

  <section class="card" aria-live="polite">
    <h2>Controller status</h2>
    <div class="status">
      <span id="statusDot" class="dot"></span>
      <div>
        <div id="statusTitle">Ready</div>
        <p id="statusMessage">Connect to the controller's Wi-Fi network.</p>
      </div>
    </div>
  </section>

  <p class="note">Glow Controller · Local Wi-Fi control</p>
</main>

<script>
const colorInput = document.getElementById('color');
const preview = document.getElementById('preview');
const hexLabel = document.getElementById('hexLabel');
const brightnessInput = document.getElementById('brightness');
const brightnessValue = document.getElementById('brightnessValue');
const statusTitle = document.getElementById('statusTitle');
const statusMessage = document.getElementById('statusMessage');
const statusDot = document.getElementById('statusDot');

let brightnessTimer = null;
let activeEffect = 'off';
let requestInProgress = false;

function setStatus(title, message, isError = false) {
  statusTitle.textContent = title;
  statusMessage.textContent = message;
  statusDot.classList.toggle('error', isError);
}

function updatePreview() {
  const color = colorInput.value.toUpperCase();
  hexLabel.textContent = color;
  preview.style.background = color;

  const glow = color + '55';
  preview.style.boxShadow = '0 0 28px ' + glow;
}

function setActiveEffect(mode) {
  activeEffect = mode;

  document.querySelectorAll('.effect-button').forEach(button => {
    const selected = button.dataset.mode === mode;
    button.classList.toggle('active', selected);
    button.setAttribute('aria-pressed', String(selected));
  });
}

async function apiRequest(path) {
  if (requestInProgress) {
    throw new Error('Another command is still being processed. Try again.');
  }

  requestInProgress = true;
  setStatus('Sending command', 'Waiting for the ESP32…');

  try {
    const response = await fetch(path, {
      method: 'GET',
      cache: 'no-store',
      signal: AbortSignal.timeout(5000)
    });

    const message = await response.text();

    if (!response.ok) {
      throw new Error('ESP32 error ' + response.status + ': ' + message);
    }

    setStatus('Command successful', message);
    return message;
  } catch (error) {
    let message;

    if (error.name === 'TimeoutError') {
      message = 'The ESP32 did not respond in time. Check Wi-Fi.';
    } else if (error instanceof TypeError) {
      message = 'Cannot reach the ESP32. Reconnect to its Wi-Fi network.';
    } else {
      message = error.message || 'Unexpected error.';
    }

    setStatus('Command failed', message, true);
    throw error;
  } finally {
    requestInProgress = false;
  }
}

async function applyColor() {
  const hex = colorInput.value.slice(1);

  try {
    await apiRequest('/api/color?hex=' + encodeURIComponent(hex));
    setActiveEffect('solid');
  } catch (_) {
    // Detailed error is already displayed in the status panel.
  }
}

async function setBrightness(value) {
  try {
    await apiRequest('/api/brightness?value=' + value);
  } catch (_) {
    // Detailed error is already displayed in the status panel.
  }
}

function queueBrightness(value) {
  clearTimeout(brightnessTimer);
  brightnessTimer = setTimeout(() => setBrightness(value), 180);
}

async function chooseEffect(mode) {
  try {
    await apiRequest('/api/effect?mode=' + encodeURIComponent(mode));
    setActiveEffect(mode);
  } catch (_) {
    // Keep the previous active indicator if the command fails.
  }
}

colorInput.addEventListener('input', updatePreview);

document.getElementById('applyColor').addEventListener('click', applyColor);

brightnessInput.addEventListener('input', () => {
  brightnessValue.textContent = brightnessInput.value + '%';
  preview.style.opacity = String(Number(brightnessInput.value) / 100);
  queueBrightness(brightnessInput.value);
});

brightnessInput.addEventListener('change', () => {
  clearTimeout(brightnessTimer);
  setBrightness(brightnessInput.value);
});

document.querySelectorAll('.effect-button').forEach(button => {
  button.addEventListener('click', () => chooseEffect(button.dataset.mode));
});

updatePreview();
setActiveEffect('off');
</script>
</body>
</html>
)rawliteral";

// ---------- BRIGHTNESS ----------
void applyBrightness() {
  uint8_t value = (uint16_t)brightnessPercent * 255 / 100;
  FastLED.setBrightness(value);
}

void showSolidColor() {
  applyBrightness();
  fill_solid(leds, NUM_LEDS, selectedColor);
  FastLED.show();
}

void turnOff() {
  FastLED.clear(true);
}

// ---------- EFFECT ENGINE ----------
void updateAnimation() {
  unsigned long now = millis();

  if (now - previousFrame < frameInterval) {
    return;
  }

  previousFrame = now;

  switch (currentEffect) {
    case EFFECT_RAINBOW:
      applyBrightness();
      fill_rainbow(leds, NUM_LEDS, rainbowHue, 7);
      rainbowHue++;
      FastLED.show();
      break;

    case EFFECT_CHASE:
      applyBrightness();
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      leds[chasePosition] = selectedColor;
      leds[(chasePosition + NUM_LEDS - 1) % NUM_LEDS] =
          selectedColor;
      chasePosition = (chasePosition + 1) % NUM_LEDS;
      FastLED.show();
      break;

    case EFFECT_BREATHE: {
      uint8_t wave = beatsin8(12, 5, 255);
      uint8_t baseBrightness =
          (uint16_t)brightnessPercent * 255 / 100;
      uint8_t breathingBrightness =
          (uint16_t)baseBrightness * wave / 255;

      fill_solid(leds, NUM_LEDS, selectedColor);
      FastLED.setBrightness(breathingBrightness);
      FastLED.show();

      // Restore the user-selected maximum brightness.
      FastLED.setBrightness(baseBrightness);
      break;
    }

    default:
      break;
  }
}

// ---------- WEB HANDLERS ----------
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleColor() {
  if (!server.hasArg("hex")) {
    server.send(400, "text/plain", "Missing color parameter.");
    return;
  }

  String hex = server.arg("hex");

  if (hex.length() != 6) {
    server.send(400, "text/plain", "Color must contain six hex digits.");
    return;
  }

  char* endPointer;
  unsigned long rgb = strtoul(hex.c_str(), &endPointer, 16);

  if (*endPointer != '\0') {
    server.send(400, "text/plain", "Color contains invalid characters.");
    return;
  }

  selectedColor = CRGB(
      (rgb >> 16) & 0xFF,
      (rgb >> 8) & 0xFF,
      rgb & 0xFF
  );

  currentEffect = EFFECT_SOLID;
  showSolidColor();

  Serial.println("Color updated from dashboard.");
  server.send(200, "text/plain", "Color applied.");
}

void handleBrightness() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing brightness parameter.");
    return;
  }

  String rawValue = server.arg("value");

  if (rawValue.length() == 0) {
    server.send(400, "text/plain", "Brightness cannot be empty.");
    return;
  }

  for (unsigned int i = 0; i < rawValue.length(); i++) {
    if (!isDigit(rawValue[i])) {
      server.send(400, "text/plain", "Brightness must be a number.");
      return;
    }
  }

  int value = rawValue.toInt();

  if (value < 1 || value > 100) {
    server.send(400, "text/plain", "Brightness must be between 1 and 100.");
    return;
  }

  brightnessPercent = value;
  applyBrightness();

  if (currentEffect == EFFECT_SOLID) {
    showSolidColor();
  } else if (currentEffect == EFFECT_OFF) {
    turnOff();
  }

  Serial.printf("Brightness: %d%%\n", brightnessPercent);
  server.send(200, "text/plain", "Brightness set to " +
              String(brightnessPercent) + "%.");
}

void handleEffect() {
  if (!server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing effect parameter.");
    return;
  }

  String mode = server.arg("mode");

  if (mode == "solid") {
    currentEffect = EFFECT_SOLID;
    showSolidColor();
  } else if (mode == "rainbow") {
    currentEffect = EFFECT_RAINBOW;
    rainbowHue = 0;
  } else if (mode == "chase") {
    currentEffect = EFFECT_CHASE;
    chasePosition = 0;
  } else if (mode == "breathe") {
    currentEffect = EFFECT_BREATHE;
  } else if (mode == "off") {
    currentEffect = EFFECT_OFF;
    turnOff();
  } else {
    server.send(400, "text/plain", "Unknown effect. Choose a supported mode.");
    return;
  }

  Serial.print("Effect selected: ");
  Serial.println(mode);

  server.send(200, "text/plain", "Effect changed to " + mode + ".");
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(
      leds, NUM_LEDS
  );

  brightnessPercent = 20;
  applyBrightness();
  turnOff();

  WiFi.mode(WIFI_AP);

  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("ERROR: Failed to start Wi-Fi access point.");

    while (true) {
      delay(1000);
    }
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/color", HTTP_GET, handleColor);
  server.on("/api/brightness", HTTP_GET, handleBrightness);
  server.on("/api/effect", HTTP_GET, handleEffect);

  server.onNotFound([]() {
    server.send(404, "text/plain",
                "Not found. Open the dashboard at /");
  });

  server.begin();

  Serial.println();
  Serial.println("ESP32 LED Controller V2 is ready.");
  Serial.print("Wi-Fi name: ");
  Serial.println(AP_SSID);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
}

// ---------- MAIN LOOP ----------
void loop() {
  server.handleClient();
  updateAnimation();
}
