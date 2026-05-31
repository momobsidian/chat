# 🚀 ESP32 Online Notification System - Complete Guide

## 📋 Overview

This system sends real-time online notifications from your chat app to an ESP32 device anywhere in the world via WebSocket. When someone comes online, the ESP32:
- ✅ Displays notification on OLED screen
- ✅ Plays notification sound on buzzer
- ✅ Blinks LED
- ✅ Logs to Serial Monitor

## 🎯 What You Have

### ✅ Server Side (Already Configured)
- **Primary URL**: `wss://saayem.qzz.io/esp32`
- **Fallback**: Render hosted URL (configurable)
- **Protocol**: Secure WebSocket (WSS) over HTTPS
- **Port**: 443
- **Auto-reconnect**: Every 5 seconds
- **Heartbeat**: PING every 30 seconds

### ✅ ESP32 Client (Ready to Upload)
- **WiFi**: Pre-configured (Sayem / Sayem123)
- **Components**: OLED Display + Buzzer + LED
- **Features**: Auto-reconnect, server fallback, visual feedback
- **File**: `esp32-notification-client.ino`

## 🛠️ Hardware Setup

### Required Components
1. ESP32 Development Board
2. OLED Display (SSD1306, 128x64, I2C, 4-pin)
3. Buzzer (3-pin, active or passive)
4. Jumper wires
5. USB cable

### Wiring (See ESP32-WIRING.md for diagram)
```
OLED Display:
  VCC → ESP32 3.3V
  GND → ESP32 GND
  SDA → ESP32 GPIO 21
  SCL → ESP32 GPIO 22

Buzzer:
  VCC → ESP32 3.3V (or 5V for louder)
  GND → ESP32 GND
  Signal → ESP32 GPIO 25

LED: Built-in on GPIO 2
```

## 💻 Software Setup

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Add ESP32 Board Support
1. Open Arduino IDE
2. File → Preferences
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Tools → Board → Boards Manager
5. Search "esp32" and install "esp32 by Espressif Systems"

### 3. Install Required Libraries
Go to Sketch → Include Library → Manage Libraries, then install:
- **WebSocketsClient** by Markus Sattler
- **ArduinoJson** by Benoit Blanchon
- **Adafruit SSD1306** by Adafruit
- **Adafruit GFX Library** by Adafruit

### 4. Configure (Already Done!)
The sketch is pre-configured with:
```cpp
WiFi SSID: "Sayem"
WiFi Password: "Sayem123"
Primary Server: "saayem.qzz.io"
Fallback Server: "your-app.onrender.com" (update if needed)
```

### 5. Upload
1. Connect ESP32 via USB
2. Open `esp32-notification-client.ino`
3. Select: Tools → Board → ESP32 Dev Module
4. Select: Tools → Port → (your COM port)
5. Click Upload (→)

### 6. Monitor
1. Open: Tools → Serial Monitor
2. Set baud rate: 115200
3. Watch for connection status

## 📊 Expected Output

### Serial Monitor on Startup
```
╔════════════════════════════════════════╗
║  ESP32 Online Notification Client     ║
║  with OLED Display & Buzzer            ║
╚════════════════════════════════════════╝

Connecting to WiFi: Sayem ........ ✓
  IP Address: 192.168.1.xxx
  Signal: -45 dBm
✓ OLED Display initialized
✓ WebSocket client initialized
  Connecting to: wss://saayem.qzz.io:443/esp32

✓ WebSocket Connected!
  Server: wss://saayem.qzz.io:443/esp32
✓ Server welcomed ESP32
```

### When Someone Comes Online
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

### OLED Display Shows
```
┌──────────────────────┐
│ ONLINE!              │
│                      │
│ Sayem                │
│                      │
│ Device: V2430        │
│ Time: 6:35:28 PM     │
└──────────────────────┘
```

### Buzzer Plays
- Pleasant 4-tone notification melody
- Duration: ~600ms

### LED Blinks
- 5 rapid blinks (100ms on/off)

## 🔧 Customization

### Change WiFi (if needed)
```cpp
const char* ssid = "YourWiFi";
const char* password = "YourPassword";
```

### Change Buzzer Pin
```cpp
#define BUZZER_PIN 25  // Change to your pin
```

### Change OLED I2C Address
```cpp
#define SCREEN_ADDRESS 0x3C  // Try 0x3D if not working
```

### Add Render Fallback URL
```cpp
const char* ws_host_fallback = "your-app.onrender.com";
```

### Customize Notification Tone
```cpp
void playNotificationTone() {
  playBuzzer(1000, 100);  // Frequency, Duration
  delay(50);
  playBuzzer(1500, 100);
  delay(50);
  playBuzzer(2000, 150);
  delay(100);
  playBuzzer(1500, 200);
}
```

## 🧪 Testing

### Test Without ESP32
Run the test script on your computer:
```bash
node test-esp32-connection.js
```

This simulates an ESP32 connection and shows what messages it receives.

### Test Individual Components
See `ESP32-WIRING.md` for component test sketches.

## 🐛 Troubleshooting

### WiFi Won't Connect
- ✓ Check SSID and password
- ✓ Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- ✓ Check signal strength
- ✓ Restart ESP32

### WebSocket Won't Connect
- ✓ Verify server is running
- ✓ Check Cloudflare tunnel is active
- ✓ Try fallback server
- ✓ Check firewall/network

### OLED Not Working
- ✓ Check I2C address (0x3C or 0x3D)
- ✓ Verify wiring (SDA/SCL)
- ✓ Run I2C scanner
- ✓ Check power (3.3V)

### Buzzer Not Working
- ✓ Check buzzer type (active/passive)
- ✓ Verify GPIO 25 connection
- ✓ Try different frequency
- ✓ Check power

### No Notifications
- ✓ Check Serial Monitor for connection
- ✓ Verify someone is coming online
- ✓ Check server logs
- ✓ Test with test script

## 📈 Performance

- **Latency**: 100-500ms from event to notification
- **Reconnection**: Automatic every 5 seconds
- **Heartbeat**: Server PING every 30 seconds
- **Memory**: ~50KB RAM usage
- **Power**: ~80mA active
- **Range**: Anywhere with internet!

## 🔒 Security

- ✅ WSS (WebSocket Secure) encryption
- ✅ HTTPS (port 443)
- ✅ Cloudflare tunnel protection
- ✅ Read-only access (ESP32 only receives)

## 🌐 Server Fallback

The ESP32 automatically switches between:
1. **Primary**: `saayem.qzz.io` (Cloudflare tunnel)
2. **Fallback**: Render hosted URL (if primary fails)

To set Render URL, update:
```cpp
const char* ws_host_fallback = "your-app.onrender.com";
```

## 📁 File Structure

```
chat/
├── esp32-notification-client.ino    # Main ESP32 sketch
├── ESP32-SETUP.md                   # Detailed setup guide
├── ESP32-WIRING.md                  # Wiring diagrams
├── ESP32-QUICK-START.md             # Quick start guide
├── ESP32-COMPLETE-GUIDE.md          # This file
├── test-esp32-connection.js         # Test script
└── server.js                        # Server with ESP32 support
```

## 🎯 Next Steps

1. ✅ Wire up components (see ESP32-WIRING.md)
2. ✅ Upload sketch (already configured!)
3. ✅ Open Serial Monitor
4. ✅ Watch for connection
5. ✅ Test by coming online in chat
6. ✅ Enjoy real-time notifications!

## 💡 Ideas for Extension

### Add More Actions
```cpp
void handlePresenceUpdate(JsonDocument& doc) {
  // Turn on relay
  digitalWrite(RELAY_PIN, HIGH);
  
  // Send to MQTT
  mqttClient.publish("home/notification", "User online");
  
  // Log to SD card
  logToSD(user, device, time);
  
  // Send HTTP request
  sendHTTPNotification(user);
}
```

### Add Deep Sleep
```cpp
// Sleep between notifications to save power
esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 seconds
esp_deep_sleep_start();
```

### Add OTA Updates
```cpp
#include <ArduinoOTA.h>

void setup() {
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  webSocket.loop();
}
```

## 📞 Support

If you encounter issues:
1. Check Serial Monitor output
2. Verify wiring (ESP32-WIRING.md)
3. Test components individually
4. Check server logs
5. Run test script

## 🎉 Success!

Your ESP32 is now a global notification receiver! It will alert you whenever someone comes online in your chat app, from anywhere in the world with internet access.

---

**Primary Server**: `wss://saayem.qzz.io/esp32`  
**WiFi**: Sayem / Sayem123  
**Components**: OLED + Buzzer + LED  
**Status**: Ready to Upload! 🚀
