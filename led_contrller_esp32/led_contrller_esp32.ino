#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "secrets.h"

// Glow Controller V2: existing home Wi-Fi dashboard + BLE control.
// Keep secrets.h local and do not commit it to a public repository.

// ---------------- Hardware ----------------
#define LED_PIN 5
#define NUM_LEDS 41
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

enum Effect { EFFECT_SOLID, EFFECT_RAINBOW, EFFECT_CHASE, EFFECT_BREATHE, EFFECT_OFF };
Effect currentEffect = EFFECT_OFF;
CRGB selectedColor = CRGB::Red;
uint8_t brightnessPercent = 20;
uint8_t rainbowHue = 0;
uint8_t chasePosition = 0;
unsigned long previousFrame = 0;
const unsigned long frameInterval = 20;
unsigned long lastWiFiRetry = 0;
const unsigned long wifiRetryInterval = 10000;

// ---------------- BLE ----------------
#define BLE_DEVICE_NAME "Glow-Controller"
#define BLE_SERVICE_UUID        "7a1e0001-8f2b-4d3c-9a6e-1b2c3d4e5f60"
#define BLE_COMMAND_UUID        "7a1e0002-8f2b-4d3c-9a6e-1b2c3d4e5f60"
#define BLE_STATUS_UUID         "7a1e0003-8f2b-4d3c-9a6e-1b2c3d4e5f60"

BLECharacteristic* bleStatusCharacteristic = nullptr;
QueueHandle_t bleCommandQueue = nullptr;

// The BLE callback queues commands; LED/FastLED changes happen in loop().
struct BleCommandMessage {
  char text[64];
};

class GlowServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    Serial.println("BLE phone connected.");
    if (bleStatusCharacteristic) {
      bleStatusCharacteristic->setValue("CONNECTED");
      bleStatusCharacteristic->notify();
    }
  }

  void onDisconnect(BLEServer* server) override {
    Serial.println("BLE phone disconnected; restarting advertising.");
    if (bleStatusCharacteristic) {
      bleStatusCharacteristic->setValue("DISCONNECTED");
    }
    BLEDevice::startAdvertising();
  }
};

class GlowCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    // Works with ESP32 BLE library versions where getValue() returns either
    // Arduino String or std::string because both provide c_str().
    auto received = characteristic->getValue();
    String command = String(received.c_str());
    command.trim();

    if (command.length() == 0 || command.length() >= sizeof(BleCommandMessage::text)) {
      Serial.println("BLE command rejected: empty or too long.");
      return;
    }

    BleCommandMessage message = {};
    command.toCharArray(message.text, sizeof(message.text));
    if (bleCommandQueue && xQueueSend(bleCommandQueue, &message, 0) != pdTRUE) {
      Serial.println("BLE command queue full; command dropped.");
    }
  }
};

// ---------------- Shared LED functions ----------------
uint8_t brightness8() {
  return (uint16_t)brightnessPercent * 255 / 100;
}

void applyBrightness() {
  FastLED.setBrightness(brightness8());
}

void showSolidColor() {
  applyBrightness();
  fill_solid(leds, NUM_LEDS, selectedColor);
  FastLED.show();
}

void turnOff() {
  FastLED.clear(true);
}

const char* effectName(Effect effect) {
  switch (effect) {
    case EFFECT_SOLID: return "solid";
    case EFFECT_RAINBOW: return "rainbow";
    case EFFECT_CHASE: return "chase";
    case EFFECT_BREATHE: return "breathe";
    case EFFECT_OFF: return "off";
    default: return "unknown";
  }
}

void publishBleStatus(const String& status) {
  if (bleStatusCharacteristic) {
    bleStatusCharacteristic->setValue(status.c_str());
    // Notify is harmless if the phone has not subscribed; it may simply not receive it.
    bleStatusCharacteristic->notify();
  }
}

void updateAnimation() {
  unsigned long now = millis();
  if (now - previousFrame < frameInterval) return;
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
      leds[(chasePosition + NUM_LEDS - 1) % NUM_LEDS] = selectedColor;
      chasePosition = (chasePosition + 1) % NUM_LEDS;
      FastLED.show();
      break;

    case EFFECT_BREATHE: {
      uint8_t wave = beatsin8(12, 5, 255);
      uint8_t base = brightness8();
      uint8_t breathing = (uint16_t)base * wave / 255;
      fill_solid(leds, NUM_LEDS, selectedColor);
      FastLED.setBrightness(breathing);
      FastLED.show();
      FastLED.setBrightness(base);
      break;
    }

    case EFFECT_SOLID:
    case EFFECT_OFF:
    default:
      break;
  }
}

// ---------------- Shared command handlers ----------------
bool parseHexColor(String hex, CRGB& result) {
  hex.trim();
  if (hex.startsWith("#")) hex.remove(0, 1);
  if (hex.length() != 6) return false;

  for (unsigned int i = 0; i < hex.length(); i++) {
    if (!isHexadecimalDigit(hex[i])) return false;
  }

  char* endPointer = nullptr;
  unsigned long rgb = strtoul(hex.c_str(), &endPointer, 16);
  if (endPointer == hex.c_str() || *endPointer != '\0') return false;
  result = CRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
  return true;
}

bool setColorCommand(const String& hex, String& response) {
  CRGB parsedColor;
  if (!parseHexColor(hex, parsedColor)) {
    response = "ERROR: color must be 6 hex digits, e.g. COLOR=FF0000";
    return false;
  }
  selectedColor = parsedColor;
  currentEffect = EFFECT_SOLID;
  showSolidColor();
  response = "OK: color=" + hex;
  return true;
}

bool setBrightnessCommand(const String& rawValue, String& response) {
  if (rawValue.length() == 0) {
    response = "ERROR: use BRIGHTNESS=1..100";
    return false;
  }
  for (unsigned int i = 0; i < rawValue.length(); i++) {
    if (!isDigit(rawValue[i])) {
      response = "ERROR: brightness must be numeric, 1..100";
      return false;
    }
  }
  int value = rawValue.toInt();
  if (value < 1 || value > 100) {
    response = "ERROR: brightness must be 1..100";
    return false;
  }

  brightnessPercent = (uint8_t)value;
  if (currentEffect == EFFECT_SOLID) showSolidColor();
  else if (currentEffect == EFFECT_OFF) turnOff();
  else applyBrightness();
  response = "OK: brightness=" + String(brightnessPercent);
  return true;
}

bool setEffectCommand(String mode, String& response) {
  mode.trim();
  mode.toLowerCase();

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
    response = "ERROR: effect must be solid, rainbow, chase, breathe, or off";
    return false;
  }
  response = "OK: effect=" + mode;
  return true;
}

void processBleCommand(const String& rawCommand) {
  String command = rawCommand;
  command.trim();
  String upper = command;
  upper.toUpperCase();
  String response;
  bool success = false;

  if (upper.startsWith("COLOR=")) {
    success = setColorCommand(command.substring(6), response);
  } else if (upper.startsWith("BRIGHTNESS=")) {
    success = setBrightnessCommand(command.substring(11), response);
  } else if (upper.startsWith("EFFECT=")) {
    success = setEffectCommand(command.substring(7), response);
  } else if (upper == "STATUS") {
    response = String("STATE: effect=") + effectName(currentEffect) +
               ", brightness=" + String(brightnessPercent) +
               ", color=#" + String(selectedColor.r < 16 ? "0" : "") + String(selectedColor.r, HEX) +
               String(selectedColor.g < 16 ? "0" : "") + String(selectedColor.g, HEX) +
               String(selectedColor.b < 16 ? "0" : "") + String(selectedColor.b, HEX);
    response.toUpperCase();
    success = true;
  } else {
    response = "ERROR: use COLOR=RRGGBB, BRIGHTNESS=1..100, EFFECT=mode, or STATUS";
  }

  Serial.print("BLE command: ");
  Serial.println(command);
  Serial.println(response);
  publishBleStatus(response);
  (void)success;
}

void setupBLE() {
  bleCommandQueue = xQueueCreate(8, sizeof(BleCommandMessage));
  if (!bleCommandQueue) {
    Serial.println("ERROR: could not create BLE command queue.");
    return;
  }

  BLEDevice::init(BLE_DEVICE_NAME);
  BLEServer* bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new GlowServerCallbacks());
  BLEService* service = bleServer->createService(BLE_SERVICE_UUID);

  BLECharacteristic* commandCharacteristic = service->createCharacteristic(
    BLE_COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setCallbacks(new GlowCommandCallbacks());

  bleStatusCharacteristic = service->createCharacteristic(
    BLE_STATUS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  bleStatusCharacteristic->addDescriptor(new BLE2902());
  bleStatusCharacteristic->setValue("READY");

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("BLE ready. Device name: Glow-Controller");
  Serial.println("BLE commands: COLOR=FF0000, BRIGHTNESS=40, EFFECT=rainbow, STATUS");
}

void processQueuedBleCommands() {
  if (!bleCommandQueue) return;
  BleCommandMessage message;
  while (xQueueReceive(bleCommandQueue, &message, 0) == pdTRUE) {
    processBleCommand(String(message.text));
  }
}

// ---------------- Existing Wi-Fi dashboard ----------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#10131b"><title>Glow | LED Controller</title>
<style>
:root{color-scheme:dark;--bg:#10131b;--panel:#1b202c;--panel2:#242b3a;--text:#f4f6ff;--muted:#aab3c5;--accent:#8c9cff;--border:#343c4e;--success:#78e3ac;--danger:#ff9292}
*{box-sizing:border-box}body{margin:0;padding:22px 15px 36px;background:radial-gradient(ellipse at top,#252d48 0%,var(--bg) 55%);color:var(--text);font-family:system-ui,sans-serif}
main{max-width:520px;margin:auto}header{margin-bottom:22px}.eyebrow{color:var(--accent);font-size:12px;letter-spacing:2px;font-weight:800}
h1{margin:8px 0;font-size:clamp(27px,7vw,36px)}p{color:var(--muted);line-height:1.5}
.card{background:rgba(27,32,44,.96);border:1px solid var(--border);border-radius:20px;padding:19px;margin:15px 0}
.card h2{margin:0 0 15px;font-size:17px}.preview{height:105px;border-radius:15px;background:#f00;border:1px solid #ffffff24;transition:background .12s,box-shadow .12s,opacity .12s}
.preview-caption,.range-label{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:13px 0}
#hexLabel{font-family:monospace;font-size:14px;font-weight:700}.pill{border:1px solid var(--border);background:var(--panel2);color:var(--muted);padding:5px 10px;border-radius:30px;font-size:12px}
label{display:block;margin:14px 0 9px;font-weight:650;font-size:14px}input[type=color]{display:block;width:100%;height:50px;padding:3px;border:1px solid var(--border);border-radius:12px;background:var(--panel2)}
input[type=range]{width:100%;accent-color:var(--accent)}#brightnessValue{color:var(--accent);font-size:21px;font-weight:800}
button{border:1px solid var(--border);border-radius:12px;padding:13px 12px;color:var(--text);background:var(--panel2);font:inherit;font-weight:700;cursor:pointer;touch-action:manipulation}
button:active{transform:scale(.98)}.primary{background:var(--accent);color:#101426;border-color:transparent}.full{width:100%;margin-top:12px}
.effects{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.effect-button{text-align:left;min-height:70px}
.effect-button span{display:block;margin-top:5px;font-size:12px;font-weight:400;color:var(--muted)}
.effect-button.active{border-color:var(--accent);background:#343d5c;box-shadow:inset 0 0 0 1px var(--accent)}
.effect-button.active:after{content:" ✓ ACTIVE";display:block;margin-top:7px;color:var(--success);font-size:10px;font-weight:800}
.status{display:flex;align-items:flex-start;gap:10px}.dot{width:9px;height:9px;margin-top:6px;border-radius:50%;background:var(--success);flex:0 0 auto}.dot.error{background:var(--danger)}
#statusTitle{font-weight:750}#statusMessage{margin:4px 0 0;font-size:13px;overflow-wrap:anywhere}.note{text-align:center;font-size:12px;margin-top:22px}
@media(max-width:360px){body{padding:15px 10px 28px}.card{padding:15px}.effects{grid-template-columns:1fr}}
</style></head><body><main>
<header><div class="eyebrow">ESP32 · SMART LIGHTING</div><h1>Glow Controller</h1><p>Control all 41 LEDs from your phone over your home Wi-Fi.</p></header>
<section class="card"><h2>Light preview</h2><div id="preview" class="preview" role="img" aria-label="Selected LED color preview"></div>
<div class="preview-caption"><span id="hexLabel">#FF0000</span><span class="pill">41 LEDs</span></div>
<label for="color">Choose your color</label><input id="color" type="color" value="#ff0000"><button id="applyColor" class="primary full">Apply color</button></section>
<section class="card"><div class="range-label"><h2>Brightness</h2><span id="brightnessValue">20%</span></div>
<input id="brightness" type="range" min="1" max="100" value="20" aria-label="Brightness percentage">
<div class="range-label"><span class="pill">Low</span><span class="pill">High</span></div><p style="font-size:12px">Preview updates instantly; the LED strip updates after a short pause.</p></section>
<section class="card"><h2>Lighting effects</h2><p>Choose an effect. The active mode is highlighted.</p><div class="effects">
<button class="effect-button" data-mode="solid">Solid color<span>Your selected color</span></button>
<button class="effect-button" data-mode="rainbow">Rainbow<span>Flowing colors</span></button>
<button class="effect-button" data-mode="chase">Color chase<span>Moving light</span></button>
<button class="effect-button" data-mode="breathe">Breathe<span>Gentle pulsing</span></button>
<button class="effect-button" data-mode="off">Off<span>Turn all LEDs off</span></button>
</div></section>
<section class="card" aria-live="polite"><h2>Controller status</h2><div class="status"><span id="statusDot" class="dot"></span><div><div id="statusTitle">Ready</div><p id="statusMessage">Connect this device to the same home Wi-Fi as the ESP32.</p></div></div></section>
<p class="note">Glow Controller V2 · Local Wi-Fi + BLE control</p></main>
<script>
const colorInput=document.getElementById('color'),preview=document.getElementById('preview'),hexLabel=document.getElementById('hexLabel');
const brightnessInput=document.getElementById('brightness'),brightnessValue=document.getElementById('brightnessValue');
const statusTitle=document.getElementById('statusTitle'),statusMessage=document.getElementById('statusMessage'),statusDot=document.getElementById('statusDot');
let brightnessTimer=null,requestInProgress=false;
function setStatus(title,message,error=false){statusTitle.textContent=title;statusMessage.textContent=message;statusDot.classList.toggle('error',error)}
function updatePreview(){const c=colorInput.value.toUpperCase();hexLabel.textContent=c;preview.style.background=c;preview.style.boxShadow='0 0 28px '+c+'55'}
function setActiveEffect(mode){document.querySelectorAll('.effect-button').forEach(b=>{const on=b.dataset.mode===mode;b.classList.toggle('active',on);b.setAttribute('aria-pressed',String(on))})}
async function apiRequest(path){if(requestInProgress)throw new Error('A command is still being processed. Try again.');requestInProgress=true;setStatus('Sending command','Waiting for the ESP32…');try{const r=await fetch(path,{cache:'no-store',signal:AbortSignal.timeout(5000)});const t=await r.text();if(!r.ok)throw new Error('ESP32 error '+r.status+': '+t);setStatus('Command successful',t);return t}catch(e){const msg=e.name==='TimeoutError'?'ESP32 timed out. Check Wi-Fi and its IP address.':e instanceof TypeError?'Cannot reach ESP32. Ensure both devices use the same home Wi-Fi.':(e.message||'Unexpected error.');setStatus('Command failed',msg,true);throw e}finally{requestInProgress=false}}
async function applyColor(){try{await apiRequest('/api/color?hex='+encodeURIComponent(colorInput.value.slice(1)));setActiveEffect('solid')}catch(e){}}
async function setBrightness(v){try{await apiRequest('/api/brightness?value='+encodeURIComponent(v))}catch(e){}}
function queueBrightness(v){clearTimeout(brightnessTimer);brightnessTimer=setTimeout(()=>setBrightness(v),250)}
async function chooseEffect(mode){try{await apiRequest('/api/effect?mode='+encodeURIComponent(mode));setActiveEffect(mode)}catch(e){}}
colorInput.addEventListener('input',updatePreview);document.getElementById('applyColor').addEventListener('click',applyColor);
brightnessInput.addEventListener('input',()=>{brightnessValue.textContent=brightnessInput.value+'%';preview.style.opacity=String(Number(brightnessInput.value)/100);queueBrightness(brightnessInput.value)});
brightnessInput.addEventListener('change',()=>{clearTimeout(brightnessTimer);setBrightness(brightnessInput.value)});
document.querySelectorAll('.effect-button').forEach(b=>b.addEventListener('click',()=>chooseEffect(b.dataset.mode)));
updatePreview();setActiveEffect('off');
</script></body></html>
)rawliteral";

// ---------------- HTTP handlers (original routes preserved) ----------------
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleColor() {
  if (!server.hasArg("hex")) {
    server.send(400, "text/plain", "Missing color parameter.");
    return;
  }
  CRGB parsedColor;
  if (!parseHexColor(server.arg("hex"), parsedColor)) {
    server.send(400, "text/plain", "Color must contain six valid hex digits.");
    return;
  }
  selectedColor = parsedColor;
  currentEffect = EFFECT_SOLID;
  showSolidColor();
  Serial.println("Color updated over Wi-Fi.");
  server.send(200, "text/plain", "Color applied.");
}

void handleBrightness() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing brightness parameter.");
    return;
  }
  String raw = server.arg("value");
  if (raw.length() == 0) {
    server.send(400, "text/plain", "Brightness cannot be empty.");
    return;
  }
  for (unsigned int i = 0; i < raw.length(); i++) {
    if (!isDigit(raw[i])) {
      server.send(400, "text/plain", "Brightness must be a number.");
      return;
    }
  }
  int value = raw.toInt();
  if (value < 1 || value > 100) {
    server.send(400, "text/plain", "Brightness must be between 1 and 100.");
    return;
  }
  brightnessPercent = (uint8_t)value;
  if (currentEffect == EFFECT_SOLID) showSolidColor();
  else if (currentEffect == EFFECT_OFF) turnOff();
  else applyBrightness();
  Serial.printf("Brightness: %u%%\n", brightnessPercent);
  server.send(200, "text/plain", "Brightness set to " + String(brightnessPercent) + "%.");
}

void handleEffect() {
  if (!server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing effect parameter.");
    return;
  }
  String mode = server.arg("mode");
  String response;
  if (!setEffectCommand(mode, response)) {
    server.send(400, "text/plain", "Unknown effect. Choose solid, rainbow, chase, breathe, or off.");
    return;
  }
  Serial.print("Effect selected over Wi-Fi: ");
  Serial.println(mode);
  server.send(200, "text/plain", "Effect changed to " + mode + ".");
}

void connectToHomeWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println();
  Serial.print("Connecting to home Wi-Fi: ");
  Serial.println(WIFI_SSID);

  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    Serial.print("Dashboard URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection timed out; will retry automatically.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setDither(false);
  brightnessPercent = 20;
  applyBrightness();
  turnOff();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/color", HTTP_GET, handleColor);
  server.on("/api/brightness", HTTP_GET, handleBrightness);
  server.on("/api/effect", HTTP_GET, handleEffect);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found. Open the dashboard at /");
  });

  connectToHomeWiFi();
  server.begin();
  Serial.println("HTTP server started.");

  setupBLE();
}

void loop() {
  server.handleClient();
  processQueuedBleCommands();
  updateAnimation();

  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastWiFiRetry >= wifiRetryInterval) {
      lastWiFiRetry = now;
      Serial.println("Wi-Fi disconnected; retrying...");
      WiFi.reconnect();
    }
  }
  delay(1);
}
