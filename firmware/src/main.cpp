// main.cpp (FULL DROP-IN) — KeyMBO AtomS3 HID (WiFi + BLE) + REAL USB re-enumeration HID Reset
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <M5AtomS3.h>

// Native USB (ESP32-S3)
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"

// BLE (NimBLE)
#include <NimBLEDevice.h>

// ---- TinyUSB (for true disconnect/reconnect) ----
#if __has_include("tusb.h")
extern "C" {
  #include "tusb.h"
}
  #define KMBO_HAS_TUSB 1
#else
  #define KMBO_HAS_TUSB 0
#endif

USBHIDKeyboard Keyboard;
USBHIDMouse Mouse;

// Optional: Consumer control (volume/mute) if present
#if __has_include("USBHIDConsumerControl.h")
  #include "USBHIDConsumerControl.h"
  USBHIDConsumerControl Consumer;
  #define KMBO_HAS_CONSUMER 1
#else
  #define KMBO_HAS_CONSUMER 0
#endif

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- USB Identity Strings ----------------
static const char* USB_MANUFACTURER = "KeyMBO";
static const char* USB_PRODUCT      = "KeyMBO Field HID";
static const char* USB_SERIAL       = "KMBO-ATOM-0001";

// ---------------- Wi-Fi AP ----------------
static String AP_SSID;
static String AP_PIN;

static String makeSSID() {
  uint64_t mac = ESP.getEfuseMac();
  uint16_t s = (uint16_t)(mac & 0xFFFF);
  char buf[32];
  snprintf(buf, sizeof(buf), "ATOM-HID-%04X", s);
  return String(buf);
}

static String makePIN() {
  uint32_t r = (uint32_t)esp_random();
  char buf[8];
  snprintf(buf, sizeof(buf), "%06u", (unsigned)(r % 1000000));
  return String(buf);
}

// ---------------- Display / Title state ----------------
enum class TitleMode {
  IDLE,
  WIFI,         // solid green
  BLE_PAIRING,  // flashing blue
  BLE_CONNECTED // solid blue
};

static TitleMode currentTitleMode = TitleMode::IDLE;

// flash control
static bool titleFlashOn = true;
static uint32_t lastFlashAt = 0;
static const uint32_t FLASH_MS = 420;

// Draw just the title bar (top strip)
static void drawTitleBar() {
  AtomS3.Display.fillRect(0, 0, 240, 24, BLACK);

  uint16_t color = WHITE;
  switch (currentTitleMode) {
    case TitleMode::WIFI:
      color = GREEN;
      break;
    case TitleMode::BLE_CONNECTED:
      color = BLUE;
      break;
    case TitleMode::BLE_PAIRING:
      color = titleFlashOn ? BLUE : BLACK; // flash: show/hide
      break;
    case TitleMode::IDLE:
    default:
      color = WHITE;
      break;
  }

  AtomS3.Display.setTextColor(color, BLACK);
  AtomS3.Display.setTextSize(2);
  AtomS3.Display.setCursor(2, 2);
  AtomS3.Display.println("KeyMBO HID");
}

// ---------------- Display helpers ----------------
static void dispInit() {
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  AtomS3.Display.setTextColor(WHITE, BLACK);
  AtomS3.Display.clear();
  AtomS3.Display.setCursor(0, 0);
}

static void dispAPInfo(const IPAddress& ip) {
  AtomS3.Display.clear();

  // Title bar (colored / flashing handled elsewhere)
  drawTitleBar();

  // SSID
  AtomS3.Display.setTextColor(WHITE, BLACK);
  AtomS3.Display.setTextSize(1);
  AtomS3.Display.setCursor(2, 26);
  AtomS3.Display.print("SSID: ");
  AtomS3.Display.println(AP_SSID);

  // Big PIN
  AtomS3.Display.setCursor(2, 44);
  AtomS3.Display.println("PIN:");
  AtomS3.Display.setTextSize(3);
  AtomS3.Display.setCursor(2, 58);
  AtomS3.Display.println(AP_PIN);

  // URL
  AtomS3.Display.setTextSize(1);
  AtomS3.Display.setCursor(2, 94);
  AtomS3.Display.println("URL:");
  AtomS3.Display.setCursor(2, 106);
  AtomS3.Display.print("http://");
  AtomS3.Display.println(ip.toString());
}

// ---------------- Auth ----------------
// NOTE: Used for WebSocket only. BLE is treated as trusted once connected.
static bool checkAuthWS(const JsonDocument& doc) {
  if (!doc["pin"].is<const char*>()) return false;
  return String(doc["pin"].as<const char*>()) == AP_PIN;
}

// ---------------- Mouse helpers ----------------
static uint8_t mouseButtonFromChar(const char* b) {
  if (!b || !b[0]) return 0;
  switch (b[0]) {
    case 'l': return MOUSE_BUTTON_LEFT;
    case 'r': return MOUSE_BUTTON_RIGHT;
    case 'm': return MOUSE_BUTTON_MIDDLE;
    default:  return 0;
  }
}

template <typename T>
static auto mouseMoveWheelImpl(T& m, int8_t x, int8_t y, int8_t wheel, int)
  -> decltype(m.move(x, y, wheel), void()) {
  m.move(x, y, wheel);
}

template <typename T>
static void mouseMoveWheelImpl(T& m, int8_t x, int8_t y, int8_t wheel, long) {
  (void)wheel;
  m.move(x, y);
}

static void mouseMoveWithWheel(int dx, int dy, int wheel) {
  dx = constrain(dx, -127, 127);
  dy = constrain(dy, -127, 127);
  wheel = constrain(wheel, -127, 127);
  mouseMoveWheelImpl(Mouse, (int8_t)dx, (int8_t)dy, (int8_t)wheel, 0);
}

// ---------------- Keycode compatibility helpers ----------------
#ifndef KMBO_KEY_PRINT_SCREEN
  #ifdef KEY_PRINT_SCREEN
    #define KMBO_KEY_PRINT_SCREEN KEY_PRINT_SCREEN
  #else
    #define KMBO_KEY_PRINT_SCREEN HID_KEY_PRINT_SCREEN
  #endif
#endif

#ifndef KMBO_KEY_HOME
  #ifdef KEY_HOME
    #define KMBO_KEY_HOME KEY_HOME
  #else
    #define KMBO_KEY_HOME HID_KEY_HOME
  #endif
#endif

#ifndef KMBO_KEY_END
  #ifdef KEY_END
    #define KMBO_KEY_END KEY_END
  #else
    #define KMBO_KEY_END HID_KEY_END
  #endif
#endif

#ifndef KMBO_KEY_PAGE_UP
  #ifdef KEY_PAGE_UP
    #define KMBO_KEY_PAGE_UP KEY_PAGE_UP
  #else
    #define KMBO_KEY_PAGE_UP HID_KEY_PAGE_UP
  #endif
#endif

#ifndef KMBO_KEY_PAGE_DOWN
  #ifdef KEY_PAGE_DOWN
    #define KMBO_KEY_PAGE_DOWN KEY_PAGE_DOWN
  #else
    #define KMBO_KEY_PAGE_DOWN HID_KEY_PAGE_DOWN
  #endif
#endif

// ---------------- Consumer helpers ----------------
static bool tapMediaKey(const String& k) {
#if KMBO_HAS_CONSUMER
  uint16_t usage = 0;
  if      (k == "VOLUP") usage = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
  else if (k == "VOLDN") usage = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
  else if (k == "MUTE")  usage = HID_USAGE_CONSUMER_MUTE;
  else return false;

  Consumer.press(usage);
  delay(3);
  Consumer.release();
  return true;
#else
  (void)k;
  return false;
#endif
}

// ---------------- Keyboard helpers ----------------
static bool pressNamedKey(const String& k) {
  if (k == "VOLUP" || k == "VOLDN" || k == "MUTE") return tapMediaKey(k);

  if (k == "SHIFT")     { Keyboard.press(KEY_LEFT_SHIFT); return true; }
  if (k == "CTRL")      { Keyboard.press(KEY_LEFT_CTRL);  return true; }
  if (k == "ALT")       { Keyboard.press(KEY_LEFT_ALT);   return true; }
  if (k == "GUI")       { Keyboard.press(KEY_LEFT_GUI);   return true; }

  if (k == "ENTER")     { Keyboard.press(KEY_RETURN);     return true; }
  if (k == "TAB")       { Keyboard.press(KEY_TAB);        return true; }
  if (k == "ESC")       { Keyboard.press(KEY_ESC);        return true; }
  if (k == "BACKSPACE") { Keyboard.press(KEY_BACKSPACE);  return true; }
  if (k == "DELETE")    { Keyboard.press(KEY_DELETE);     return true; }
  if (k == "SPACE")     { Keyboard.press(' ');            return true; }

  if (k == "UP")        { Keyboard.press(KEY_UP_ARROW);    return true; }
  if (k == "DOWN")      { Keyboard.press(KEY_DOWN_ARROW);  return true; }
  if (k == "LEFT")      { Keyboard.press(KEY_LEFT_ARROW);  return true; }
  if (k == "RIGHT")     { Keyboard.press(KEY_RIGHT_ARROW); return true; }

  if (k == "HOME")      { Keyboard.press(KMBO_KEY_HOME);      return true; }
  if (k == "END")       { Keyboard.press(KMBO_KEY_END);       return true; }
  if (k == "PAGE_UP")   { Keyboard.press(KMBO_KEY_PAGE_UP);   return true; }
  if (k == "PAGE_DOWN") { Keyboard.press(KMBO_KEY_PAGE_DOWN); return true; }

  if (k == "PRINTSCREEN" || k == "PRTSCR" || k == "PRTSC") {
    Keyboard.press(KMBO_KEY_PRINT_SCREEN);
    return true;
  }

  if (k.length() >= 2 && k[0] == 'F') {
    int n = k.substring(1).toInt();
    if (n >= 1 && n <= 12) {
      Keyboard.press((uint8_t)(KEY_F1 + (n - 1)));
      return true;
    }
  }

  return false;
}

static bool releaseNamedKey(const String& k) {
  if (k == "VOLUP" || k == "VOLDN" || k == "MUTE") return true;

  if (k == "SHIFT")     { Keyboard.release(KEY_LEFT_SHIFT); return true; }
  if (k == "CTRL")      { Keyboard.release(KEY_LEFT_CTRL);  return true; }
  if (k == "ALT")       { Keyboard.release(KEY_LEFT_ALT);   return true; }
  if (k == "GUI")       { Keyboard.release(KEY_LEFT_GUI);   return true; }

  if (k == "ENTER")     { Keyboard.release(KEY_RETURN);     return true; }
  if (k == "TAB")       { Keyboard.release(KEY_TAB);        return true; }
  if (k == "ESC")       { Keyboard.release(KEY_ESC);        return true; }
  if (k == "BACKSPACE") { Keyboard.release(KEY_BACKSPACE);  return true; }
  if (k == "DELETE")    { Keyboard.release(KEY_DELETE);     return true; }
  if (k == "SPACE")     { Keyboard.release(' ');            return true; }

  if (k == "UP")        { Keyboard.release(KEY_UP_ARROW);    return true; }
  if (k == "DOWN")      { Keyboard.release(KEY_DOWN_ARROW);  return true; }
  if (k == "LEFT")      { Keyboard.release(KEY_LEFT_ARROW);  return true; }
  if (k == "RIGHT")     { Keyboard.release(KEY_RIGHT_ARROW); return true; }

  if (k == "HOME")      { Keyboard.release(KMBO_KEY_HOME);      return true; }
  if (k == "END")       { Keyboard.release(KMBO_KEY_END);       return true; }
  if (k == "PAGE_UP")   { Keyboard.release(KMBO_KEY_PAGE_UP);   return true; }
  if (k == "PAGE_DOWN") { Keyboard.release(KMBO_KEY_PAGE_DOWN); return true; }

  if (k == "PRINTSCREEN" || k == "PRTSCR" || k == "PRTSC") {
    Keyboard.release(KMBO_KEY_PRINT_SCREEN);
    return true;
  }

  if (k.length() >= 2 && k[0] == 'F') {
    int n = k.substring(1).toInt();
    if (n >= 1 && n <= 12) {
      Keyboard.release((uint8_t)(KEY_F1 + (n - 1)));
      return true;
    }
  }

  return false;
}

// ---------------- Shared reply helpers ----------------
static void wsSendJson(AsyncWebSocketClient* client, const JsonDocument& doc) {
  if (!client) return;
  String s;
  serializeJson(doc, s);
  client->text(s);
}

// ---------------- BLE (NimBLE) ----------------
static NimBLEServer* bleServer = nullptr;
static NimBLECharacteristic* bleTx = nullptr;
static NimBLECharacteristic* bleRx = nullptr;
static bool bleConnected = false;

static const char* BLE_SVC_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* BLE_RX_UUID  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // write
static const char* BLE_TX_UUID  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // notify

static void bleNotifyJson(const JsonDocument& doc) {
  if (!bleTx || !bleConnected) return;
  String out;
  serializeJson(doc, out);
  bleTx->setValue((uint8_t*)out.c_str(), out.length());
  bleTx->notify();
}

class BleServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    bleConnected = true;
    currentTitleMode = TitleMode::BLE_CONNECTED;
    titleFlashOn = true;
    drawTitleBar();
  }

  void onDisconnect(NimBLEServer*) override {
    bleConnected = false;
    currentTitleMode = TitleMode::BLE_PAIRING;
    titleFlashOn = true;
    lastFlashAt = millis();
    drawTitleBar();
    NimBLEDevice::startAdvertising();
  }
};

// ---------------- HID reset helpers ----------------

// Some cores expose USB.end(); some don’t. We feature-detect it.
template <typename T>
static auto usbEndIfPresent(T& u, int) -> decltype(u.end(), void()) { u.end(); }
template <typename T>
static void usbEndIfPresent(T&, long) { /* no-op */ }

static void releaseAllInputs() {
  Keyboard.releaseAll();

  // Mouse does NOT have releaseAll() in your current core:
  Mouse.release(MOUSE_BUTTON_LEFT);
  Mouse.release(MOUSE_BUTTON_RIGHT);
  Mouse.release(MOUSE_BUTTON_MIDDLE);

#if KMBO_HAS_CONSUMER
  Consumer.release();
#endif
}

static uint32_t lastHIDResetAt = 0;
static const uint32_t HID_RESET_COOLDOWN_MS = 1200;

// ✅ Updated: TRUE detach/attach via TinyUSB (tud_disconnect/tud_connect) + USB stack restart
static void doHIDResetNow() {
  uint32_t now = millis();
  if (now - lastHIDResetAt < HID_RESET_COOLDOWN_MS) return;
  lastHIDResetAt = now;

  releaseAllInputs();
  delay(10);

#if KMBO_HAS_TUSB
  // Real “unplug/replug” from host POV
  tud_disconnect();
  delay(350);
  tud_connect();
  delay(350);
#endif

  // Also restart the Arduino USB wrapper if available
  usbEndIfPresent(USB, 0);
  delay(120);

  USB.begin();
  delay(80);

  // Re-begin HID interfaces
  Keyboard.begin();
  Mouse.begin();
#if KMBO_HAS_CONSUMER
  Consumer.begin();
#endif

  // Visual cue
  AtomS3.Display.fillRect(0, 0, 240, 24, BLACK);
  AtomS3.Display.setTextColor(WHITE, BLACK);
  AtomS3.Display.setTextSize(2);
  AtomS3.Display.setCursor(2, 2);
  AtomS3.Display.println("HID RESET");
  delay(180);
  drawTitleBar();
}

// ---------------- Command handler (shared for WS + BLE) ----------------
enum class TransportKind { WS, BLE };

static void replyResult(TransportKind tk, AsyncWebSocketClient* wsClient,
                        const char* status, const char* msg) {
  JsonDocument out;
  out["t"] = "result";
  out["status"] = status;
  out["msg"] = msg;

  if (tk == TransportKind::WS) wsSendJson(wsClient, out);
  else                        bleNotifyJson(out);
}

static void replyHello(TransportKind tk, AsyncWebSocketClient* wsClient) {
  JsonDocument out;
  out["t"] = "hello";

  // IMPORTANT: WS requires PIN; BLE does NOT.
  out["pinRequired"] = (tk == TransportKind::WS);

  out["ssid"] = AP_SSID;
  out["ip"] = WiFi.softAPIP().toString();
  out["hasConsumer"] = (bool)KMBO_HAS_CONSUMER;
  out["hasHID"] = true;
  out["mode"] = (tk == TransportKind::BLE) ? "ble" : "ws";

  if (tk == TransportKind::WS) wsSendJson(wsClient, out);
  else                        bleNotifyJson(out);
}

static void handleJsonCommandShared(TransportKind tk, AsyncWebSocketClient* wsClient,
                                    const JsonDocument& doc) {
  const char* t = doc["t"] | "";
  if (!t[0]) return;

  // allow "hello" without pin so clients can learn requirements
  if (strcmp(t, "hello") == 0) {
    replyHello(tk, wsClient);
    return;
  }

  // Auth:
  // - WS: require correct pin
  // - BLE: trusted, no pin required
  if (tk == TransportKind::WS) {
    if (!checkAuthWS(doc)) {
      replyResult(tk, wsClient, "err", "bad_pin");
      return;
    }
  }

  // HID reset command (WS+BLE)
  if (strcmp(t, "hidreset") == 0) {
    doHIDResetNow();
    replyResult(tk, wsClient, "ok", "hid_reset");
    return;
  }

  if (strcmp(t, "mm") == 0) {
    int dx = doc["dx"] | 0;
    int dy = doc["dy"] | 0;
    mouseMoveWithWheel(dx, dy, 0);
    return;
  }

  if (strcmp(t, "mc") == 0) {
    const char* b = doc["b"] | "l";
    int d = doc["d"] | 0;
    uint8_t btn = mouseButtonFromChar(b);
    if (!btn) return;
    if (d) Mouse.press(btn);
    else   Mouse.release(btn);
    return;
  }

  if (strcmp(t, "ms") == 0) {
    int v = doc["v"] | 0;
    mouseMoveWithWheel(0, 0, v);
    return;
  }

  if (strcmp(t, "txt") == 0) {
    const char* s = doc["s"] | "";
    if (s[0]) Keyboard.print(s);
    return;
  }

  if (strcmp(t, "k") == 0) {
    String key = String((const char*)(doc["k"] | ""));
    bool down = (doc["d"] | 0) ? true : false;

    if (key.length() == 1) {
      char c = key[0];
      down ? Keyboard.press(c) : Keyboard.release(c);
      return;
    }

    down ? pressNamedKey(key) : releaseNamedKey(key);
    return;
  }

  if (strcmp(t, "combo") == 0) {
    JsonArrayConst mods = doc["mods"].is<JsonArrayConst>() ? doc["mods"].as<JsonArrayConst>() : JsonArrayConst();
    const char* keyStr = doc["k"] | "";
    String key = String(keyStr);

    for (JsonVariantConst v : mods) {
      if (v.is<const char*>()) pressNamedKey(String(v.as<const char*>()));
    }

    delay(5);

    if (key.length() == 1) {
      Keyboard.write(key[0]);
    } else {
      pressNamedKey(key);
      delay(5);
      releaseNamedKey(key);
    }

    delay(5);

    for (JsonVariantConst v : mods) {
      if (v.is<const char*>()) releaseNamedKey(String(v.as<const char*>()));
    }
    return;
  }
}

// ---------------- WebSocket ----------------
static void onWSEvent(AsyncWebSocket* /*server*/, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;

  static char buf[768];
  if (len >= sizeof(buf)) return;
  memcpy(buf, data, len);
  buf[len] = '\0';

  JsonDocument doc;
  if (deserializeJson(doc, buf) != DeserializationError::Ok) return;

  handleJsonCommandShared(TransportKind::WS, client, doc);
}

// ---------------- BLE RX callback ----------------
class BleRxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    if (v.empty()) return;

    JsonDocument doc;
    if (deserializeJson(doc, v) != DeserializationError::Ok) return;

    handleJsonCommandShared(TransportKind::BLE, nullptr, doc);
  }
};

static void bleInit() {
  NimBLEDevice::init("KeyMBO");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(185);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new BleServerCallbacks());

  NimBLEService* svc = bleServer->createService(BLE_SVC_UUID);

  bleTx = svc->createCharacteristic(BLE_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  bleRx = svc->createCharacteristic(
    BLE_RX_UUID,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  bleRx->setCallbacks(new BleRxCallbacks());

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_SVC_UUID);
  adv->setScanResponse(true);
  adv->start();

  // Start in "pairing" visual state (flashing blue)
  currentTitleMode = TitleMode::BLE_PAIRING;
  titleFlashOn = true;
  lastFlashAt = millis();
  drawTitleBar();
}

// ---------------- Setup / Loop ----------------
void setup() {
  dispInit();
  Serial.begin(115200);
  delay(100);

  // USB strings + start
  USB.manufacturerName(USB_MANUFACTURER);
  USB.productName(USB_PRODUCT);
  USB.serialNumber(USB_SERIAL);
  USB.begin();

  Keyboard.begin();
  Mouse.begin();
#if KMBO_HAS_CONSUMER
  Consumer.begin();
#endif

  // FS
  LittleFS.begin(true);

  // Wi-Fi AP + PIN
  AP_SSID = makeSSID();
  AP_PIN  = makePIN();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID.c_str(), nullptr);

  // Wi-Fi up => green title (BLE init will switch to flashing blue)
  currentTitleMode = TitleMode::WIFI;
  titleFlashOn = true;
  drawTitleBar();

  IPAddress ip = WiFi.softAPIP();
  dispAPInfo(ip);

  // WS
  ws.onEvent(onWSEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // WS UI still requires PIN
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["ssid"] = AP_SSID;
    doc["ip"] = WiFi.softAPIP().toString();
    doc["pinRequired"] = true;
    doc["hasConsumer"] = (bool)KMBO_HAS_CONSUMER;
    doc["hasHID"] = true; // for UI convenience
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  server.begin();

  // BLE (this will switch title into flashing blue pairing state)
  bleInit();

  // redraw full info (so SSID/PIN/URL are visible after BLE title change)
  dispAPInfo(ip);
}

void loop() {
  AtomS3.update();
  ws.cleanupClients();

  // Physical HID reset: tap AtomS3 BtnA
  if (AtomS3.BtnA.wasPressed()) {
    doHIDResetNow();
  }

  // Flash title while pairing (advertising / not connected)
  if (currentTitleMode == TitleMode::BLE_PAIRING) {
    uint32_t now = millis();
    if (now - lastFlashAt >= FLASH_MS) {
      lastFlashAt = now;
      titleFlashOn = !titleFlashOn;
      drawTitleBar();
    }
  }

  delay(5);
}