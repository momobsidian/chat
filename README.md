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
- Live Android Push: Integrated WebSocket server to push instant state updates to Android clients.
- Standalone Desktop Monitor: A dedicated `desktop-notifier.js` script that provides native OS desktop notifications.
- Security-First: Database operations are handled server-side using the Firebase Admin SDK, keeping credentials secure and out of the client interface.

## Prerequisites

- Node.js (v14 or higher recommended)
- Firebase Project with Realtime Database enabled
- Service Account Keys for Firebase

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

## Architecture

- Frontend: Native HTML, CSS, and Vanilla JavaScript for maximum performance and a smooth, native-like mobile experience.
- Backend: Node.js server handling static file delivery, WebSocket connections, and secure Firebase Admin SDK operations.
- Deployment: Designed to be easily hosted on platforms like Render or Heroku with environment variable injection.
