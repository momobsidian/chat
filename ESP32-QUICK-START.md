# ESP32 Quick Start Guide

## 🚀 Quick Setup (5 Minutes)

### 1. Hardware
- Get any ESP32 board
- Connect via USB to computer

### 2. Software
- Install Arduino IDE
- Add ESP32 board support
- Install libraries:
  - WebSocketsClient
  - ArduinoJson

### 3. Configure
Open `esp32-notification-client.ino` and change:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 4. Upload
- Select board: ESP32 Dev Module
- Select port: COM port
- Click Upload

### 5. Monitor
- Open Serial Monitor (115200 baud)
- Watch for connection success
- Wait for online notifications!

## 📡 Connection Details

**WebSocket URL**: `wss://saayem.qzz.io/esp32`  
**Protocol**: Secure WebSocket (WSS)  
**Port**: 443

## ✅ What You Get

When someone comes online:
- ✓ Real-time notification to ESP32
- ✓ User details (name, device, time)
- ✓ LED blinks 5 times
- ✓ Serial output with formatted info

## 🧪 Test Without ESP32

Run the test script on your computer:
```bash
node test-esp32-connection.js
```

This simulates an ESP32 connection and shows you what messages it will receive.

## 📖 Full Documentation

See `ESP32-SETUP.md` for:
- Detailed setup instructions
- Customization examples
- Troubleshooting guide
- Advanced features

## 🎯 Use Cases

- **Home Automation**: Turn on lights when someone comes online
- **Notification Display**: Show on OLED/LCD screen
- **Alert System**: Sound buzzer or alarm
- **IoT Integration**: Forward to other devices
- **Monitoring**: Log activity to SD card
- **Remote Control**: Trigger actions anywhere in the world

## 🔧 Server Status

Your server is already configured and running at:
- **Production**: `https://saayem.qzz.io`
- **WebSocket**: `wss://saayem.qzz.io/esp32`

The server automatically:
- ✓ Accepts ESP32 connections
- ✓ Sends presence updates
- ✓ Keeps connection alive (heartbeat)
- ✓ Auto-reconnects if dropped

## 📊 Architecture

```
Chat App (Firebase) 
    ↓
Node.js Server (saayem.qzz.io)
    ↓
WebSocket (WSS)
    ↓
Cloudflare Tunnel
    ↓
Internet
    ↓
Your ESP32 (anywhere in the world!)
```

## 🎉 That's It!

Your ESP32 will now receive real-time notifications whenever someone comes online in your chat app, from anywhere in the world!
