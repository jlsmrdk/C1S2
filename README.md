# Darkride Vehicle RFID Publisher

This project reads RFID tag data using a PN532 NFC module connected to an ESP32 and publishes the tag’s location (text data stored on the tag) to an MQTT broker.
It is designed for use in a trackless dark ride vehicle setup where each vehicle reads location blocks and publishes them over the network.

---

## Overview

The ESP32 connects to a WiFi network, authenticates to an MQTT broker, and listens for nearby RFID tags using the PN532.  
When a tag is detected:
- It reads the data stored on the tag.
- Converts it to readable text.
- Publishes the text to an MQTT topic.

To prevent unnecessary traffic, the ESP32 only publishes when the location has changed.

---

## Hardware Requirements

- ESP32 development board (tested with ESP32-DevKitV1)
- PN532 NFC module
  - Communication mode: I²C
  - Connections:

| PN532 Pin | ESP32 Pin | Description |
|------------|------------|--------------|
| VCC        | 3.3V       | Power        |
| GND        | GND        | Ground       |
| SDA        | GPIO 21    | I²C Data     |
| SCL        | GPIO 22    | I²C Clock    |

---

## How It Works

1. Connects to WiFi using provided SSID and password.  
2. Connects to an MQTT broker with authentication.  
3. Initializes the PN532 reader.  
4. Waits for RFID tags to be presented.  
5. Reads and decodes tag data (block 4 for Mifare Classic, pages 4–7 for Ultralight).  
6. Publishes the resulting text (for example, `A1`, `Scene_2`) to the configured MQTT topic.

---

## Example Output

Serial Monitor:
```
ESP32 + PN532 + MQTT auth (location only)
PN532 detected. FW 1.6
WiFi OK. IP: 192.168.10.45
Connecting to MQTT 192.168.10.1:1883 with auth
MQTT connected
Waiting for RFID-tag
Published location: A3
Same location as last time, skipping publish.
```

---

## Configuration

Before uploading the code, set the following variables at the top of the sketch:

```cpp
// WiFi Configuration
const char* WIFI_SSID     = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASS";

// MQTT Configuration
const char* MQTT_SERVER = "MQTT_IP";      // e.g., "192.168.10.1"
const uint16_t MQTT_PORT = 1883;          // Usually 1883
const char* MQTT_TOPIC = "MQTT_TOPIC";    // e.g., "darkride/vehicles/V01/location"
const char* MQTT_USER  = "MQTT_USER";
const char* MQTT_PASS  = "MQTT_PASS";
```

| Variable | Description | Example |
|-----------|-------------|----------|
| `WIFI_SSID` | Your WiFi network name | `"YOUR_SSID"` |
| `WIFI_PASSWORD` | WiFi password | `"YOUR_WIFI_PASSWORD"` |
| `MQTT_SERVER` | IP address of your MQTT broker | `"for example 192.168.20.10"` |
| `MQTT_PORT` | MQTT port (usually `1883`) | `1883` |
| `MQTT_TOPIC` | Topic to publish location data to | `"darkride/vehicles/V01/location"` |
| `MQTT_USER` | MQTT username | `"YOUR_MQTT_USERNAME"` |
| `MQTT_PASS` | MQTT password | `"YOUR_MQTT_PASSWORD"` |

---

## Dependencies

Install these Arduino libraries (via Library Manager):

- Adafruit PN532  
- PubSubClient

---

## Testing

1. Flash the code to your ESP32 using Arduino IDE.  
2. Open Serial Monitor at 115200 baud.  
3. Wait until it connects to WiFi and MQTT.  
4. Present an RFID tag.  
5. Check:
   - Serial output for detected location.
   - MQTT broker for messages published on your topic.

Example MQTT message:
```
Topic: darkride/vehicles/V01/location
Payload: A3
```

---

## Notes

- The code uses I²C mode for PN532 (default pins: SDA = 21, SCL = 22).  
- Only ASCII-readable data on the tag is published.  
- If the same tag remains near the reader, it will not re-publish until a new tag is scanned.  
- If the PN532 is not detected, the code halts with a “PN532 not found” message.