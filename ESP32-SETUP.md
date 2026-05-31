# ESP32 Online Notification Setup Guide

This guide will help you set up an ESP32 to receive real-time online notifications from your chat server via WebSocket over the internet.

## 📋 Requirements

### Hardware
- ESP32 development board (any variant: ESP32, ESP32-S2, ESP32-C3, etc.)
- USB cable for programming
- WiFi network with internet access

### Software
- Arduino IDE (1.8.x or 2.x)
- Required Arduino Libraries:
  - `WiFi` (built-in with ESP32 board package)
  - `WebSocketsClient` by Markus Sattler
  - `ArduinoJson` by Benoit Blanchon

## 🔧 Arduino IDE Setup

### 1. Install ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install "esp32 by Espressif Systems"

### 2. Install Required Libraries

1. Go to **Sketch → Include Library → Manage Libraries**
2. Search and install:
   - **WebSocketsClient** by Markus Sattler (version 2.3.6 or later)
   - **ArduinoJson** by Benoit Blanchon (version 6.x)

## 📝 Configuration

### 1. Open the ESP32 Sketch

Open `esp32-notification-client.ino` in Arduino IDE.

### 2. Configure WiFi Credentials

Find these lines and update with your WiFi details:

```cpp
const char* ssid = "YOUR_WIFI_SSID";           // Your WiFi name
const char* password = "YOUR_WIFI_PASSWORD";   // Your WiFi password
```

### 3. WebSocket Configuration (Already Set)

The sketch is pre-configured to connect to your server:

```cpp
const char* ws_host = "saayem.qzz.io";
const uint16_t ws_port = 443;  // HTTPS port for Cloudflare tunnel
const char* ws_path = "/esp32";  // ESP32-specific endpoint
```

## 🚀 Upload and Test

### 1. Select Board and Port

1. Go to **Tools → Board** and select your ESP32 board (e.g., "ESP32 Dev Module")
2. Go to **Tools → Port** and select the COM port your ESP32 is connected to

### 2. Upload the Sketch

1. Click the **Upload** button (→)
2. Wait for compilation and upload to complete

### 3. Monitor Serial Output

1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see:
   ```
   ╔════════════════════════════════════════╗
   ║  ESP32 Online Notification Client     ║
   ╚════════════════════════════════════════╝

   Connecting to WiFi: YourWiFi ........ ✓
     IP Address: 192.168.1.xxx
     Signal: -45 dBm
   ✓ WebSocket client initialized
     Connecting to: wss://saayem.qzz.io:443/esp32

   ✓ WebSocket Connected!
     Server: wss://saayem.qzz.io:443/esp32
   ```

## 📊 What Happens When Someone Comes Online

When a user comes online in your chat app, the ESP32 will:

1. Receive a WebSocket message with user details
2. Print formatted information to Serial Monitor:
   ```
   ╔════════════════════════════════════════╗
   ║       PRESENCE UPDATE                  ║
   ╠════════════════════════════════════════╣
   ║ User:   Sayem                          ║
   ║ State:  online                         ║
   ║ Device: V2430                          ║
   ║ Time:   6:35:28 PM                     ║
   ╚════════════════════════════════════════╝
   ```
3. Blink the built-in LED 5 times rapidly

## 🔌 Server Configuration

The server is already configured to handle ESP32 connections. Here's what happens:

### WebSocket Endpoint
- **URL**: `wss://saayem.qzz.io/esp32`
- **Protocol**: Secure WebSocket (WSS) over HTTPS
- **Port**: 443 (standard HTTPS port)

### Message Format

**From Server to ESP32:**
```json
{
  "type": "PRESENCE_UPDATE",
  "user": "Sayem",
  "state": "online",
  "device": "V2430",
  "time": "6:35:28 PM"
}
```

**From ESP32 to Server:**
```json
{
  "type": "ESP32_HELLO",
  "device": "ESP32"
}
```

## 🎨 Customization Ideas

You can extend the ESP32 code to:

### 1. Control External Devices
```cpp
#define RELAY_PIN 4

void handlePresenceUpdate(JsonDocument& doc) {
  if (strcmp(state, "online") == 0) {
    digitalWrite(RELAY_PIN, HIGH);  // Turn on relay
    delay(5000);
    digitalWrite(RELAY_PIN, LOW);   // Turn off after 5 seconds
  }
}
```

### 2. Display on OLED/LCD
```cpp
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void handlePresenceUpdate(JsonDocument& doc) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("%s is online!", user);
  display.display();
}
```

### 3. Play Sound/Buzzer
```cpp
#define BUZZER_PIN 5

void handlePresenceUpdate(JsonDocument& doc) {
  if (strcmp(state, "online") == 0) {
    tone(BUZZER_PIN, 1000, 500);  // 1kHz for 500ms
  }
}
```

### 4. Send to Another Service
```cpp
void handlePresenceUpdate(JsonDocument& doc) {
  // Send HTTP request to another API
  // Forward to MQTT broker
  // Send to another ESP32
  // etc.
}
```

## 🐛 Troubleshooting

### ESP32 Won't Connect to WiFi
- Check SSID and password are correct
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Check signal strength (should be > -70 dBm)

### WebSocket Connection Fails
- Verify server is running: `https://saayem.qzz.io`
- Check Cloudflare tunnel is active
- Ensure port 443 is not blocked by firewall
- Try restarting the ESP32

### No Notifications Received
- Check Serial Monitor for connection status
- Verify someone is actually coming online in the chat
- Check server logs for ESP32 connection
- Ensure WebSocket connection is established (look for "✓ WebSocket Connected!")

### SSL/TLS Errors
- ESP32 might have issues with some SSL certificates
- If you get SSL errors, you may need to add certificate validation
- Or use `webSocket.begin()` instead of `beginSSL()` if using HTTP (not recommended)

## 📡 Network Requirements

- **Outbound HTTPS (443)**: Must be allowed
- **WebSocket Upgrade**: Must be supported by network
- **No Proxy**: Direct internet connection recommended
- **Stable Connection**: WiFi should be stable for persistent connection

## 🔒 Security Notes

- Connection uses WSS (WebSocket Secure) over HTTPS
- Cloudflare tunnel provides encryption
- No authentication required (add if needed for production)
- ESP32 only receives notifications (read-only)

## 📈 Performance

- **Latency**: ~100-500ms from event to ESP32 notification
- **Reconnection**: Automatic every 5 seconds if disconnected
- **Heartbeat**: Server sends PING every 30 seconds
- **Memory**: ~50KB RAM usage
- **Power**: ~80mA active, can use deep sleep between notifications

## 🎯 Next Steps

1. Test the basic setup
2. Add your custom actions (LED, relay, display, etc.)
3. Deploy ESP32 to your desired location
4. Monitor via Serial or add remote logging
5. Consider adding OTA (Over-The-Air) updates for remote management

## 📞 Support

If you encounter issues:
1. Check Serial Monitor output
2. Verify server logs
3. Test WebSocket connection manually
4. Check network connectivity

---

**Server URL**: `wss://saayem.qzz.io/esp32`  
**Protocol**: WebSocket Secure (WSS)  
**Port**: 443 (HTTPS)
