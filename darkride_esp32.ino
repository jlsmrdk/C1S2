#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID     = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

const char* MQTT_SERVER = "MQTT_IP";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "MQTT_TOPIC";
const char* MQTT_USER  = "MQTT_USER";
const char* MQTT_PASS  = "MQTT_PASS";

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

void setup() {
	Serial.begin(115200);
	delay(500);

	connectWiFi();
  	connectMQTT();
}


void loop() {

}