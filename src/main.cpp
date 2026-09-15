#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#ifdef SHOWDECK_ELECROW_5
#include "elecrow_5.h"
#else
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#endif
#include "web_ui.h"

namespace Pins {
#ifdef SHOWDECK_ELECROW_5
constexpr int backlight = 2;
constexpr int statusLed = 38;
#else
constexpr int backlight = 21;
constexpr int touchCs = 33;
constexpr int touchIrq = 36;
constexpr int touchClk = 25;
constexpr int touchMiso = 39;
constexpr int touchMosi = 32;
constexpr int setupButton = 0;
// The onboard RGB LED is common-anode/active-low on the original CYD.
constexpr int rearLedRed = 4;
constexpr int rearLedGreen = 16;
constexpr int rearLedBlue = 17;
#endif
}

#ifdef SHOWDECK_ELECROW_5
constexpr uint16_t screenW = 800;
constexpr uint16_t screenH = 480;
constexpr int16_t headerH = 46;
constexpr int16_t buttonStartX = 8;
constexpr int16_t buttonStartY = 54;
constexpr int16_t buttonStepX = 264;
constexpr int16_t buttonStepY = 141;
constexpr int16_t buttonW = 254;
constexpr int16_t buttonH = 131;
constexpr int16_t artworkW = 248;
constexpr int16_t artworkH = 125;
constexpr int16_t swipeThreshold = 150;
constexpr uint8_t headerFont = 4;
constexpr uint8_t buttonFont = 4;
constexpr uint8_t buttonRadius = 14;
#else
constexpr uint16_t screenW = 320;
constexpr uint16_t screenH = 240;
constexpr int16_t headerH = 23;
constexpr int16_t buttonStartX = 3;
constexpr int16_t buttonStartY = 26;
constexpr int16_t buttonStepX = 106;
constexpr int16_t buttonStepY = 70;
constexpr int16_t buttonW = 101;
constexpr int16_t buttonH = 65;
constexpr int16_t artworkW = 96;
constexpr int16_t artworkH = 59;
constexpr int16_t swipeThreshold = 70;
constexpr uint8_t headerFont = 2;
constexpr uint8_t buttonFont = 2;
constexpr uint8_t buttonRadius = 8;
#endif
constexpr uint8_t defaultPages = 4;
constexpr uint8_t maxPages = 12;
constexpr uint8_t buttonsPerPage = 9;
constexpr uint8_t assetWidth = 96;
constexpr uint8_t assetHeight = 59;
constexpr uint8_t legacyAssetHeight = 42;
constexpr char configPath[] = "/deck.json";
constexpr char setupSsid[] = "ShowDeck-Setup";
constexpr char setupPassword[] = "tikitime";

#ifdef SHOWDECK_ELECROW_5
Elecrow5Display tft;
#else
TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(Pins::touchCs, Pins::touchIrq);
#endif
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
JsonDocument config;
uint8_t currentPage = 0;
bool apMode = false;
bool wasTouched = false;
uint32_t touchStarted = 0;
int16_t touchStartX = 0;
int16_t touchStartY = 0;
int16_t lastTouchX = 0;
int16_t lastTouchY = 0;

struct __attribute__((packed)) EspNowMessage {
  uint32_t magic;
  uint16_t sequence;
  char source[16];
  char target[24];
  char payload[64];
};
uint16_t espNowSequence = 0;

void turnRearLedOff() {
#ifdef SHOWDECK_ELECROW_5
  pinMode(Pins::statusLed, OUTPUT);
  digitalWrite(Pins::statusLed, LOW);
#else
  pinMode(Pins::rearLedRed, OUTPUT);
  pinMode(Pins::rearLedGreen, OUTPUT);
  pinMode(Pins::rearLedBlue, OUTPUT);
  digitalWrite(Pins::rearLedRed, HIGH);
  digitalWrite(Pins::rearLedGreen, HIGH);
  digitalWrite(Pins::rearLedBlue, HIGH);
#endif
}

uint16_t rgb565(const char *hex) {
  if (!hex || hex[0] != '#') return TFT_DARKGREY;
  const uint32_t rgb = strtoul(hex + 1, nullptr, 16);
  return tft.color565((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

void defaultConfig() {
  config.clear();
  config["version"] = 3;
  config["name"] = "Show Deck";
  config["brightness"] = 220;
  config["ssid"] = "";
  config["password"] = "";
  config["mqttHost"] = "";
  config["mqttPort"] = 1883;
  config["mqttUser"] = "";
  config["mqttPassword"] = "";
  config["espnowChannel"] = 6;
  JsonArray pages = config["pages"].to<JsonArray>();
  const char *pageNames[defaultPages] = {"SHOW", "CANDLES", "WLED", "UTILITY"};
  for (uint8_t p = 0; p < defaultPages; ++p) {
    JsonObject page = pages.add<JsonObject>();
    page["name"] = pageNames[p];
    JsonArray buttons = page["buttons"].to<JsonArray>();
    for (uint8_t i = 0; i < buttonsPerPage; ++i) {
      JsonObject button = buttons.add<JsonObject>();
      button["label"] = i == 0 && p == 0 ? "SETUP" : "";
      button["icon"] = "";
      button["image"] = "";
      button["color"] = i == 0 && p == 0 ? "#9A6414" : "#315B68";
      button["type"] = i == 0 && p == 0 ? "setup" : "none";
      button["target"] = "";
      button["payload"] = "";
    }
  }
}

bool saveConfig() {
  File file = LittleFS.open(configPath, "w");
  if (!file) return false;
  const bool ok = serializeJson(config, file) > 0;
  file.close();
  return ok;
}

void loadConfig() {
  if (!LittleFS.exists(configPath)) {
    defaultConfig();
    saveConfig();
    return;
  }
  File file = LittleFS.open(configPath, "r");
  DeserializationError error = deserializeJson(config, file);
  file.close();
  if (error || !config["pages"].is<JsonArray>()) {
    defaultConfig();
    saveConfig();
    return;
  }

  bool migrated = false;
  if ((config["version"] | 1) < 3) {
    config["version"] = 3;
    const char *oldName = config["name"] | "";
    if (!strcmp(oldName, "TikiDeck") || !oldName[0]) config["name"] = "Show Deck";
    migrated = true;
  }
  for (JsonObject page : config["pages"].as<JsonArray>()) {
    for (JsonObject button : page["buttons"].as<JsonArray>()) {
      if (!button["icon"].is<const char *>()) { button["icon"] = ""; migrated = true; }
      if (!button["image"].is<const char *>()) { button["image"] = ""; migrated = true; }
    }
  }
  if (migrated) saveConfig();
}

String deviceName() { return config["name"] | "Show Deck"; }

uint8_t pageCount() {
  const size_t count = config["pages"].size();
  if (count < 1) return 1;
  return count > maxPages ? maxPages : static_cast<uint8_t>(count);
}

String deviceHostName() {
  String host = deviceName();
  host.toLowerCase();
  host.replace(" ", "-");
  String clean;
  for (size_t i = 0; i < host.length(); ++i) {
    const char c = host[i];
    if (isalnum(static_cast<unsigned char>(c)) || c == '-') clean += c;
  }
  return clean.length() ? clean : "show-deck";
}

void drawHeader() {
  tft.fillRect(0, 0, screenW, headerH, TFT_BLACK);
  tft.setTextFont(headerFont);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  JsonArray pages = config["pages"].as<JsonArray>();
  const char *pageName = pages[currentPage]["name"] | "PAGE";
  tft.drawString(pageName, 8, screenW > 320 ? 8 : 4);
  String status = apMode ? "SETUP" : (WiFi.status() == WL_CONNECTED ? "WIFI" : "OFFLINE");
  tft.setTextColor(apMode ? TFT_YELLOW : TFT_LIGHTGREY, TFT_BLACK);
  tft.drawRightString(status, screenW - 8, screenW > 320 ? 8 : 4, headerFont);
}

bool drawAsset(const char *path, int16_t x, int16_t y) {
  if (!path || !path[0] || !LittleFS.exists(path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  const size_t size = file.size();
  const uint8_t height = size == assetWidth * assetHeight * 2 ? assetHeight :
                         (size == assetWidth * legacyAssetHeight * 2 ? legacyAssetHeight : 0);
  if (!height) {
    if (file) file.close();
    return false;
  }
  uint16_t sourceRow[assetWidth];
  uint16_t outputRow[artworkW];
  int16_t outputH = artworkH;
  int16_t top = y;
#ifndef SHOWDECK_ELECROW_5
  if (height == legacyAssetHeight) {
    outputH = legacyAssetHeight;
    top += (artworkH - outputH) / 2;
  }
#endif
  // Assets are stored as little-endian RGB565 words. TFT_eSPI otherwise sends
  // their raw bytes low-first, while the ILI9341 expects the high byte first.
#ifndef SHOWDECK_ELECROW_5
  tft.setSwapBytes(true);
#endif
  tft.startWrite();
  int16_t previousSourceLine = -1;
  for (int16_t line = 0; line < outputH; ++line) {
    const int16_t sourceLine = static_cast<int32_t>(line) * height / outputH;
    if (sourceLine != previousSourceLine) {
      if (!file.seek(sourceLine * assetWidth * 2) ||
          file.read(reinterpret_cast<uint8_t *>(sourceRow), sizeof(sourceRow)) != sizeof(sourceRow)) break;
      previousSourceLine = sourceLine;
    }
    for (int16_t column = 0; column < artworkW; ++column) {
      outputRow[column] = sourceRow[static_cast<int32_t>(column) * assetWidth / artworkW];
    }
    tft.pushImage(x, top + line, artworkW, 1, outputRow);
  }
  tft.endWrite();
#ifndef SHOWDECK_ELECROW_5
  tft.setSwapBytes(false);
#endif
  file.close();
  return true;
}

void drawButton(uint8_t slot, bool pressed = false) {
  JsonArray pages = config["pages"].as<JsonArray>();
  JsonObject button = pages[currentPage]["buttons"][slot];
  const uint8_t col = slot % 3;
  const uint8_t row = slot / 3;
  const int16_t x = buttonStartX + col * buttonStepX;
  const int16_t y = buttonStartY + row * buttonStepY;
  uint16_t color = rgb565(button["color"] | "#315B68");
  if (pressed) color = tft.color565(238, 181, 74);
  tft.fillRoundRect(x, y, buttonW, buttonH, buttonRadius, color);
  tft.drawRoundRect(x, y, buttonW, buttonH, buttonRadius, pressed ? TFT_WHITE : TFT_DARKGREY);
  const bool hasImage = drawAsset(button["image"] | "", x + 3, y + 3);
  const char *label = button["label"] | "";
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(buttonFont);
  const int16_t labelY = hasImage ? y + buttonH - (screenW > 320 ? 18 : 10) : y + buttonH / 2;
  if (hasImage && label[0]) {
    tft.setTextColor(TFT_BLACK);
    tft.drawString(label, x + buttonW / 2 + 1, labelY + 1, buttonFont);
    tft.setTextColor(TFT_WHITE);
  } else {
    tft.setTextColor(TFT_WHITE, color);
  }
  tft.drawString(label, x + buttonW / 2, labelY, buttonFont);
  tft.setTextDatum(TL_DATUM);
}

void drawDeck() {
  if (currentPage >= pageCount()) currentPage = 0;
  tft.fillScreen(TFT_BLACK);
  drawHeader();
  for (uint8_t i = 0; i < buttonsPerPage; ++i) drawButton(i);
}

void setRadioChannel(uint8_t channel) {
  if (channel < 1 || channel > 13) channel = 6;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

void beginEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  esp_now_peer_info_t peer{};
  memset(peer.peer_addr, 0xff, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (!esp_now_is_peer_exist(peer.peer_addr)) esp_now_add_peer(&peer);
}

void sendEspNow(const char *target, const char *payload) {
  EspNowMessage message{};
  message.magic = 0x54494B49; // "TIKI"
  message.sequence = ++espNowSequence;
  strlcpy(message.source, deviceName().c_str(), sizeof(message.source));
  strlcpy(message.target, target && target[0] ? target : "all", sizeof(message.target));
  strlcpy(message.payload, payload ? payload : "", sizeof(message.payload));
  const uint8_t broadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  esp_now_send(broadcast, reinterpret_cast<uint8_t *>(&message), sizeof(message));
}

void ensureMqtt() {
  const char *host = config["mqttHost"] | "";
  if (!host[0] || WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  mqtt.setServer(host, config["mqttPort"] | 1883);
  const char *user = config["mqttUser"] | "";
  const char *pass = config["mqttPassword"] | "";
  const String clientId = deviceHostName();
  if (user[0]) mqtt.connect(clientId.c_str(), user, pass);
  else mqtt.connect(clientId.c_str());
}

void startSetupPortal();

void runAction(uint8_t slot) {
  JsonObject button = config["pages"][currentPage]["buttons"][slot];
  const String type = button["type"] | "none";
  const String target = button["target"] | "";
  const String payload = button["payload"] | "";
  if (type == "setup") {
    if (!apMode) startSetupPortal();
  } else if (type == "espnow") {
    sendEspNow(target.c_str(), payload.c_str());
  } else if (type == "mqtt") {
    ensureMqtt();
    if (mqtt.connected()) mqtt.publish(target.c_str(), payload.c_str());
  } else if (type == "http" && target.startsWith("http") && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(target);
    if (payload.length()) {
      http.addHeader("Content-Type", "application/json");
      http.POST(payload);
    } else http.GET();
    http.end();
  } else if (type == "page") {
    int page = target.toInt() - 1;
    if (page >= 0 && page < pageCount()) currentPage = page;
  }
}

void startSetupPortal() {
  apMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(setupSsid, setupPassword, config["espnowChannel"] | 6);
  Serial.printf("Setup portal: http://%s\n", WiFi.softAPIP().toString().c_str());
}

void connectWifi() {
  const char *ssid = config["ssid"] | "";
  const char *password = config["password"] | "";
  WiFi.mode(WIFI_STA);
  if (!ssid[0]) {
    startSetupPortal();
    return;
  }
  WiFi.setHostname(deviceHostName().c_str());
  WiFi.begin(ssid, password);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 8000) delay(100);
  if (WiFi.status() != WL_CONNECTED) startSetupPortal();
}

void setupWebServer() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", WEB_UI); });
  server.on("/api/config", HTTP_GET, [] {
    JsonDocument publicConfig = config;
    publicConfig["password"] = "";
    publicConfig["mqttPassword"] = "";
    String body;
    serializeJson(publicConfig, body);
    server.send(200, "application/json; charset=utf-8", body);
  });
  server.on("/api/config", HTTP_POST, [] {
    JsonDocument incoming;
    DeserializationError error = deserializeJson(incoming, server.arg("plain"));
    if (error || !incoming["pages"].is<JsonArray>()) {
      server.send(400, "text/plain", "Invalid configuration");
      return;
    }
    const size_t incomingPages = incoming["pages"].size();
    if (incomingPages < 1 || incomingPages > maxPages) {
      server.send(400, "text/plain", "Show Deck requires 1 to 12 pages");
      return;
    }
    if ((incoming["password"] | "")[0] == '\0') incoming["password"] = config["password"];
    if ((incoming["mqttPassword"] | "")[0] == '\0') incoming["mqttPassword"] = config["mqttPassword"];
    incoming["version"] = 3;
    config = incoming;
    if (currentPage >= pageCount()) currentPage = 0;
    if (!saveConfig()) {
      server.send(500, "text/plain", "Could not write configuration");
      return;
    }
    ledcWrite(0, constrain(config["brightness"] | 220, 20, 255));
    drawDeck();
    server.send(200, "text/plain", "Saved");
  });
  server.on("/api/asset", HTTP_GET, [] {
    String name = server.arg("name");
    if (!name.startsWith("asset_") || !name.endsWith(".rgb") || name.indexOf('/') >= 0) {
      server.send(400, "text/plain", "Invalid asset name");
      return;
    }
    File file = LittleFS.open("/" + name, "r");
    if (!file) { server.send(404, "text/plain", "Asset not found"); return; }
    server.streamFile(file, "application/octet-stream");
    file.close();
  });
  server.on("/api/asset", HTTP_POST, [] {
    String name = server.arg("name");
    String body = server.arg("plain");
    if (!name.startsWith("asset_") || !name.endsWith(".rgb") || name.indexOf('/') >= 0 ||
        body.length() != assetWidth * assetHeight * 4) {
      server.send(400, "text/plain", "Invalid 96x59 RGB565 asset");
      return;
    }
    File file = LittleFS.open("/" + name, "w");
    if (!file) { server.send(500, "text/plain", "Could not create asset"); return; }
    size_t written = 0;
    for (size_t i = 0; i < body.length(); i += 2) {
      const char pair[] = {body[i], body[i + 1], '\0'};
      const uint8_t value = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
      written += file.write(value);
    }
    file.close();
    const size_t expected = assetWidth * assetHeight * 2;
    server.send(written == expected ? 200 : 500, "text/plain", written == expected ? "Saved" : "Write failed");
  });
  server.on("/api/asset", HTTP_DELETE, [] {
    String name = server.arg("name");
    if (!name.startsWith("asset_") || !name.endsWith(".rgb") || name.indexOf('/') >= 0) {
      server.send(400, "text/plain", "Invalid asset name");
      return;
    }
    LittleFS.remove("/" + name);
    server.send(200, "text/plain", "Removed");
  });
  server.on("/api/reboot", HTTP_GET, [] {
    server.send(200, "text/plain", "Rebooting");
    delay(250);
    ESP.restart();
  });
  server.onNotFound([] {
    if (apMode) server.sendHeader("Location", "http://192.168.4.1/");
    server.send(apMode ? 302 : 404, "text/plain", apMode ? "" : "Not found");
  });
  server.begin();
}

bool readTouch(int16_t &x, int16_t &y) {
#ifdef SHOWDECK_ELECROW_5
  return readElecrowTouch(x, y);
#else
  if (!touch.touched()) return false;
  TS_Point point = touch.getPoint();
  // Defaults for the original CYD in landscape. Adjust after calibration if needed.
  x = constrain(map(point.x, 200, 3800, 0, screenW), 0, screenW - 1);
  y = constrain(map(point.y, 240, 3800, 0, screenH), 0, screenH - 1);
  return true;
#endif
}

int8_t buttonAt(int16_t x, int16_t y) {
  if (x < buttonStartX || y < buttonStartY) return -1;
  const int8_t col = (x - buttonStartX) / buttonStepX;
  const int8_t row = (y - buttonStartY) / buttonStepY;
  if (col < 0 || col > 2 || row < 0 || row > 2) return -1;
  const int16_t localX = x - (buttonStartX + col * buttonStepX);
  const int16_t localY = y - (buttonStartY + row * buttonStepY);
  if (localX >= buttonW || localY >= buttonH) return -1;
  return row * 3 + col;
}

void handleTouch() {
  int16_t x, y;
  const bool touchedNow = readTouch(x, y);
  if (touchedNow && !wasTouched) {
    wasTouched = true;
    touchStarted = millis();
    touchStartX = x;
    touchStartY = y;
    lastTouchX = x;
    lastTouchY = y;
    const int8_t slot = buttonAt(x, y);
    if (slot >= 0) drawButton(slot, true);
  } else if (touchedNow && wasTouched) {
    lastTouchX = x;
    lastTouchY = y;
  } else if (!touchedNow && wasTouched) {
    wasTouched = false;
    const int16_t deltaX = lastTouchX - touchStartX;
    if (abs(deltaX) > swipeThreshold) {
      const uint8_t count = pageCount();
      currentPage = deltaX < 0 ? (currentPage + 1) % count : (currentPage + count - 1) % count;
    } else {
      const int8_t slot = buttonAt(touchStartX, touchStartY);
      if (slot >= 0) runAction(slot);
    }
    drawDeck();
  }
}

void setup() {
  // Do this before starting networking or the display so the rear LED does not
  // remain illuminated during the relatively long boot sequence.
  turnRearLedOff();
  Serial.begin(115200);
#ifndef SHOWDECK_ELECROW_5
  pinMode(Pins::setupButton, INPUT_PULLUP);
#endif
  LittleFS.begin(true);
  loadConfig();

#ifdef SHOWDECK_ELECROW_5
  // Keep the large RGB panel dark until its timing controller is configured.
  pinMode(Pins::backlight, OUTPUT);
  digitalWrite(Pins::backlight, LOW);
#endif
  tft.init();
#ifdef SHOWDECK_ELECROW_5
  tft.setRotation(0);
  initElecrowTouch();
#else
  tft.setRotation(1);

  touchSpi.begin(Pins::touchClk, Pins::touchMiso, Pins::touchMosi, Pins::touchCs);
  touch.begin(touchSpi);
  touch.setRotation(1);
#endif
  ledcSetup(0, 5000, 8);
  ledcAttachPin(Pins::backlight, 0);
  ledcWrite(0, constrain(config["brightness"] | 220, 20, 255));

#ifdef SHOWDECK_ELECROW_5
  const bool forceSetup = false;
#else
  const bool forceSetup = digitalRead(Pins::setupButton) == LOW;
#endif
  connectWifi();
  if (forceSetup && !apMode) startSetupPortal();
  if (WiFi.status() != WL_CONNECTED) setRadioChannel(config["espnowChannel"] | 6);
  beginEspNow();
  setupWebServer();

  ArduinoOTA.setHostname(deviceHostName().c_str());
  ArduinoOTA.begin();
  drawDeck();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  ensureMqtt();
  mqtt.loop();
  handleTouch();
  delay(8);
}
