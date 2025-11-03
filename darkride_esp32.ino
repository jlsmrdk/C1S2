#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <PubSubClient.h>

//WIFI
const char* WIFI_SSID     = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASS";

//MQTT
const char* MQTT_SERVER = "MQTT_IP";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "MQTT_TOPIC";
const char* MQTT_USER  = "MQTT_USER";
const char* MQTT_PASS  = "MQTT_PASS";

//PN532
#define PN532_IRQ   2
#define PN532_RESET 3

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

String lastLocation = "";

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > 15000) {
      Serial.println();
      Serial.println("WiFi timeout, retrying");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      t0 = millis();
    }
  }
  Serial.println();
  Serial.print("WiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  if (mqttClient.connected()) return;

  Serial.print("Connecting to MQTT ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.println(" with auth");

  unsigned long t0 = millis();
  while (!mqttClient.connected()) {
    String clientId = "ESP32-PN532-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool ok = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
    if (ok) {
      Serial.println("MQTT connected");
      break;
    }
    if (millis() - t0 > 10000) {
      Serial.print(" state=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying");
      t0 = millis();
    }
    delay(500);
  }
}

String bytesToPrintableString(const uint8_t *buf, size_t len) {
  String out = "";
  for (size_t i = 0; i < len; i++) {
    uint8_t c = buf[i];
    if (c >= 32 && c <= 126) out += (char)c;
    else if (c == 0x00) break;
  }
  while (out.length() && out[0] == ' ') out.remove(0, 1);
  while (out.length() && out[out.length() - 1] == ' ') out.remove(out.length() - 1, 1);
  if (out.length() == 0) return String("[no-readable-data]");
  return out;
}

String readClassicLocation(uint8_t *uid, uint8_t uidLen) {
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLen, 4, 0, keya)) {
    Serial.println("Classic auth failed");
    return String("[auth-failed]");
  }
  uint8_t data[16];
  if (!nfc.mifareclassic_ReadDataBlock(4, data)) {
    Serial.println("Classic read failed");
    return String("[read-failed]");
  }
  return bytesToPrintableString(data, 16);
}

String readUltralightLocation() {
  uint8_t buf[16];
  uint8_t page[4];
  for (uint8_t p = 4; p <= 7; p++) {
    if (!nfc.mifareultralight_ReadPage(p, page)) {
      Serial.print("Ultralight read failed on page "); Serial.println(p);
      return String("[read-failed]");
    }
    for (uint8_t i = 0; i < 4; i++) buf[(p - 4) * 4 + i] = page[i];
  }
  return bytesToPrintableString(buf, 16);
}

void publishLocation(const String &location) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  if (location == lastLocation) {
    Serial.println("Same location as last time, skipping publish.");
    return;
  }

  bool ok = mqttClient.publish(MQTT_TOPIC, location.c_str());
  if (ok) {
    Serial.print("Published location: "); Serial.println(location);
    lastLocation = location;
  } else {
    Serial.println("Failed to publish location");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("ESP32 + PN532 + MQTT auth (location only)");

  Wire.begin(21, 22);
  Wire.setClock(400000);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found");
    while (1) delay(10);
  }
  Serial.print("PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("FW "); Serial.print((versiondata >> 16) & 0xFF);
  Serial.print("."); Serial.println((versiondata >> 8) & 0xFF);

  nfc.SAMConfig();
  connectWiFi();
  connectMQTT();

  Serial.println("Waiting for RFID-tag");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  uint8_t uid[7];
  uint8_t uidLen;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
    String location = "[unknown]";
    if (uidLen == 4) {
      location = readClassicLocation(uid, uidLen);
    } else if (uidLen == 7) {
      location = readUltralightLocation();
    } else {
      location = readUltralightLocation();
    }

    publishLocation(location);
    delay(1000);
  }
  delay(50);
}