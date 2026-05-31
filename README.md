# Real-time Chat Application

A high-performance, real-time messaging application built with Node.js and Firebase Realtime Database. The project features a modern user interface, robust server-side architecture, and an extensive notification system that integrates with multiple third-party services.

## Features

- Real-time Messaging: Instant message delivery and synchronization using Firebase Realtime Database.
- Live Presence Tracking: Monitor when users come online or go offline in real-time.
- Multi-Channel Notifications: Get alerted of server events and user presence via:
  - Telegram Bot
  - Discord Webhooks
  - Pushover
  - Firebase Cloud Messaging (FCM) for Android
  - **ESP32 IoT Devices** (NEW!)
- Live Android Push: Integrated WebSocket server to push instant state updates to Android clients.
- **ESP32 Global Notifications**: Send real-time notifications to ESP32 devices anywhere in the world via WebSocket with OLED display and buzzer support.
- Standalone Desktop Monitor: A dedicated `desktop-notifier.js` script that provides native OS desktop notifications.
- Security-First: Database operations are handled server-side using the Firebase Admin SDK, keeping credentials secure and out of the client interface.

## 🆕 ESP32 IoT Notification System

Get real-time notifications on an ESP32 device with OLED display and buzzer when someone comes online!

### Quick Start
1. Wire up ESP32 with OLED display and buzzer (see `ESP32-WIRING.md`)
2. Upload `esp32-notification-client.ino` (pre-configured with WiFi)
3. Watch notifications appear globally!

### Documentation
- **Quick Start**: `ESP32-QUICK-START.md` - Get running in 5 minutes
- **Complete Guide**: `ESP32-COMPLETE-GUIDE.md` - Full documentation
- **Wiring**: `ESP32-WIRING.md` - Hardware setup and diagrams
- **Detailed Setup**: `ESP32-SETUP.md` - Advanced configuration

### Features
- ✅ OLED display shows user, device, and time
- ✅ Buzzer plays notification melody
- ✅ LED blinks on notification
- ✅ Auto-reconnect with server fallback
- ✅ Works anywhere with internet
- ✅ Pre-configured WiFi (Sayem / Sayem123)

### WebSocket Endpoints
- **Primary**: `wss://saayem.qzz.io/esp32`
- **Fallback**: Render hosted URL (configurable)

## Prerequisites

- Node.js (v14 or higher recommended)
- Firebase Project with Realtime Database enabled
- Service Account Keys for Firebase
- (Optional) ESP32 board with OLED display and buzzer for IoT notifications

## Installation

1. Clone the repository:
```bash
git clone https://github.com/shajedah/chat.git
cd chat
```

2. Install dependencies:
```bash
npm install
```

3. Setup your Firebase credentials:
Place your Firebase service account JSON files in the root directory:
- `obsidian-firebase.json` (Main database credentials)
- `android-firebase.json` (FCM credentials for Android push notifications)

## Environment Variables

Create a `.env` file in the root directory based on `.env.example`. When deploying to a cloud provider like Render, configure the following variables in your dashboard:

- `FIREBASE_SERVICE_ACCOUNT`: The raw JSON string of your main Firebase service account.
- `ANDROID_FIREBASE_SERVICE_ACCOUNT`: The raw JSON string of your FCM service account.
- `TELEGRAM_BOT_TOKEN`: Your Telegram Bot API token.
- `TELEGRAM_CHAT_ID`: Your personal Telegram user ID.
- `DISCORD_WEBHOOK`: Discord channel webhook URL.
- `PUSHOVER_APP_TOKEN`: Pushover API token.
- `PUSHOVER_USER_KEY`: Pushover user key.

## Usage

Start the main server:
```bash
node server.js
```
The server will start on port 3000 (or the port specified in your environment) and serve the chat interface at `http://localhost:3000`.

To run the standalone desktop presence monitor on your personal computer:
```bash
node desktop-notifier.js
```

To test ESP32 WebSocket connection:
```bash
node test-esp32-connection.js
```

## Architecture

- Frontend: Native HTML, CSS, and Vanilla JavaScript for maximum performance and a smooth, native-like mobile experience.
- Backend: Node.js server handling static file delivery, WebSocket connections, and secure Firebase Admin SDK operations.
- WebSocket Server: Dual-endpoint WebSocket server for Android and ESP32 clients with automatic reconnection and heartbeat.
- IoT Integration: ESP32 support with OLED display and buzzer for physical notifications.
- Deployment: Designed to be easily hosted on platforms like Render or Heroku with environment variable injection.
