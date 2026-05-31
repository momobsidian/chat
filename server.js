const http = require('http');
const fs = require('fs');
const path = require('path');
const https = require('https');
const admin = require('firebase-admin');
const WebSocket = require('ws');

const PORT = process.env.PORT || 3000;

// ── Logger Utility ───────────────────────────────────────────────────────────
const log = {
  info: (msg) => console.log(`\x1b[36m[INFO]\x1b[0m   ${msg}`),
  success: (msg) => console.log(`\x1b[32m[OK]\x1b[0m     ${msg}`),
  warn: (msg) => console.log(`\x1b[33m[WARN]\x1b[0m   ${msg}`),
  error: (msg, err = '') => console.error(`\x1b[31m[ERROR]\x1b[0m  ${msg}`, err),
  system: (msg) => console.log(`\x1b[35m[SYSTEM]\x1b[0m ${msg}`),
  event: (msg) => console.log(`\x1b[34m[EVENT]\x1b[0m  ${msg}`)
};

// ── Service account from env var (safe for hosting) ──────────────────────────
let serviceAccount;
if (process.env.FIREBASE_SERVICE_ACCOUNT) {
  serviceAccount = JSON.parse(process.env.FIREBASE_SERVICE_ACCOUNT);
} else {
  serviceAccount = require('./obsidian-firebase.json');
}

const DISCORD_WEBHOOK = process.env.DISCORD_WEBHOOK ||
  'https://discord.com/api/webhooks/1496835267363999816/JmG7RaLcsLgkqA_sid64hZ9EkdJdB4UnW0A_lhm2-FnxQ5Dx_bJ6aSmAkrk3AkUkKNJc';

// ── Firebase Admin init (for database) ──────────────────────────────────────
admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: 'https://obsidian-8234e-default-rtdb.firebaseio.com'
});

const db = admin.database();

// ── Firebase Admin for FCM (separate account for Android app) ───────────────
let fcmServiceAccount;
let fcmAdmin = null;

try {
  // Try to load from environment variable first (for Render/hosting)
  if (process.env.ANDROID_FIREBASE_SERVICE_ACCOUNT) {
    fcmServiceAccount = JSON.parse(process.env.ANDROID_FIREBASE_SERVICE_ACCOUNT);
    log.info('Loaded Android Firebase config from environment variable');
  } else {
    // Fallback to local file for development
    fcmServiceAccount = require('./android-firebase.json');
    log.info('Loaded Android Firebase config from local file');
  }

  fcmAdmin = admin.initializeApp({
    credential: admin.credential.cert(fcmServiceAccount)
  }, 'fcm-app'); // Named app instance
  log.success('FCM Firebase Admin initialized successfully');
} catch (error) {
  log.error('FCM Firebase Admin initialization failed:', error.message);
}

// ── Discord notifier ─────────────────────────────────────────────────────────
function sendDiscord(message) {
  const body = JSON.stringify({ content: message });
  const url = new URL(DISCORD_WEBHOOK);

  const req = https.request({
    hostname: url.hostname,
    path: url.pathname + url.search,
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Content-Length': Buffer.byteLength(body)
    }
  }, (res) => {
    let responseData = '';

    res.on('data', (chunk) => {
      responseData += chunk;
    });

    res.on('end', () => {
      if (res.statusCode === 204 || res.statusCode === 200) {
        log.success('Discord notification sent');
      } else {
        log.error(`Discord error (${res.statusCode}):`, responseData);
      }
    });
  });

  req.on('error', (err) => {
    log.error('Discord request failed:', err.message);
  });

  req.write(body);
  req.end();
}

// ── Telegram notifier ────────────────────────────────────────────────────────
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN || '8905063068:AAGyo7VbGC3iVqLJFow_oDNHm4prt8x_Q8o';
const TELEGRAM_CHAT_ID = process.env.TELEGRAM_CHAT_ID || '8210884096';

function sendTelegram(message) {
  if (!TELEGRAM_BOT_TOKEN || !TELEGRAM_CHAT_ID) return;

  const body = JSON.stringify({
    chat_id: TELEGRAM_CHAT_ID,
    text: message,
    parse_mode: 'HTML'
  });

  const req = https.request(`https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Content-Length': Buffer.byteLength(body)
    }
  }, (res) => {
    let responseData = '';
    res.on('data', (chunk) => { responseData += chunk; });
    res.on('end', () => {
      if (res.statusCode === 200) {
        log.success('Telegram notification sent');
      } else {
        log.error(`Telegram error (${res.statusCode}):`, responseData);
      }
    });
  });

  req.on('error', (err) => {
    log.error('Telegram request failed:', err.message);
  });

  req.write(body);
  req.end();
}

// ── Pushover notifier ────────────────────────────────────────────────────────
const PUSHOVER_APP_TOKEN = process.env.PUSHOVER_APP_TOKEN || 'arp6snbxdi3cnpywnpdaunpezz6ubv';
const PUSHOVER_USER_KEY = process.env.PUSHOVER_USER_KEY || 'ut7vut3v8xrquj9sjn7g4qmxvf6beo';

function sendPushover(title, message) {
  if (!PUSHOVER_APP_TOKEN || !PUSHOVER_USER_KEY) return;

  const postData = new URLSearchParams({
    token: PUSHOVER_APP_TOKEN,
    user: PUSHOVER_USER_KEY,
    message: message,
    title: title
  }).toString();

  const req = https.request({
    hostname: 'api.pushover.net',
    path: '/1/messages.json',
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
      'Content-Length': Buffer.byteLength(postData)
    }
  }, (res) => {
    if (res.statusCode === 200) {
      log.success('Pushover notification sent');
    } else {
      log.error(`Pushover error (${res.statusCode})`);
    }
  });

  req.on('error', (err) => {
    log.error('Pushover request failed:', err.message);
  });

  req.write(postData);
  req.end();
}

// ── FCM notifier (uses separate Firebase account) ───────────────────────────
async function sendFCM(title, body, topic = 'all_users') {
  if (!fcmAdmin) {
    log.warn('FCM not available - skipping notification');
    return;
  }

  const message = {
    data: {
      title: title,
      body: body
    },
    android: {
      priority: 'high'
    },
    topic: topic
  };

  try {
    const response = await fcmAdmin.messaging().send(message);
    log.success(`FCM notification sent: ${response}`);
    return response;
  } catch (error) {
    log.error('FCM error:', error.message);
  }
}

// ── 🔥 NEW: WebSocket SERVER (Android Live Push) ─────────────────────────────
// We will attach the WebSocket server to the main HTTP server on PORT 3000 below.

const androidClients = new Set();
const esp32Clients = new Set();


// push helper for Android
function pushToAndroid(data) {
  const payload = JSON.stringify(data);

  for (const ws of androidClients) {
    if (ws.readyState === 1) {
      ws.send(payload);
    }
  }
}

// push helper for ESP32
function pushToESP32(data) {
  const payload = JSON.stringify(data);
  
  for (const ws of esp32Clients) {
    if (ws.readyState === 1) {
      ws.send(payload);
      log.success(`Sent to ESP32: ${data.type}`);
    }
  }
}

// ── MIME types ───────────────────────────────────────────────────────────────
const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
};

// ── Static file server ───────────────────────────────────────────────────────
const server = http.createServer((req, res) => {
  if (req.method === 'POST' && req.url === '/api/login-visit') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      try {
        const data = JSON.parse(body);
        const device = data.device || 'Unknown';
        const time = new Date().toLocaleTimeString();
        
        log.event(`VISIT      |   Login Page     | Device: ${device} | Time: ${time}`);
        sendDiscord(`🚨 **Login Page Visited**\nDevice: ${device}\nTime: ${time}`);
        sendPushover('Login Page Visited', `Device: ${device} | Time: ${time}`);
        if (typeof sendTelegram === 'function') {
          sendTelegram(`🚨 <b>Login Page Visited</b>\nDevice: ${device}\nTime: ${time}`);
        }
        if (typeof sendFCM === 'function') {
          sendFCM('Login Page Visited', `Device: ${device} | Time: ${time}`);
        }
        
        // ESP32 push for login visit
        pushToESP32({
          type: "LOGIN_VISIT",
          device,
          time
        });
      } catch (e) {
        log.error('Failed to parse login-visit', e.message);
      }
      res.writeHead(200);
      res.end('OK');
    });
    return;
  }

  let filePath = req.url === '/' ? '/index.html' : req.url.split('?')[0];
  const fullPath = path.join(__dirname, filePath);

  fs.readFile(fullPath, (err, data) => {
    if (err) {
      res.writeHead(404);
      return res.end('404 Not Found');
    }

    const ext = path.extname(fullPath).toLowerCase();
    res.writeHead(200, {
      'Content-Type': MIME[ext] || 'application/octet-stream'
    });
    res.end(data);
  });
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws, req) => {
  const url = req.url;
  
  // Check if it's an ESP32 client
  if (url === '/esp32') {
    log.event('ESP32 client connected via WebSocket');
    esp32Clients.add(ws);
    
    ws.on('message', (message) => {
      try {
        const data = JSON.parse(message);
        if (data.type === 'ESP32_HELLO') {
          log.success(`ESP32 identified: ${data.device || 'Unknown'}`);
          ws.send(JSON.stringify({ type: 'WELCOME', message: 'Connected to notification server' }));
        } else if (data.type === 'PONG') {
          log.info('ESP32 PONG received');
        }
      } catch (e) {
        log.error('ESP32 message parse error:', e.message);
      }
    });
    
    ws.on('close', () => {
      log.event('ESP32 client disconnected');
      esp32Clients.delete(ws);
    });
    
    ws.on('error', (error) => {
      log.error('ESP32 WebSocket error:', error.message);
      esp32Clients.delete(ws);
    });
    
  } else {
    // Android client (default)
    log.event('Android client connected via WebSocket');
    androidClients.add(ws);
    
    ws.on('close', () => {
      log.event('Android client disconnected');
      androidClients.delete(ws);
    });
    
    ws.on('error', (error) => {
      log.error('Android WebSocket error:', error.message);
      androidClients.delete(ws);
    });
  }
});

server.listen(PORT, () => {
  log.system(`Server & WebSocket running on port ${PORT}`);

  // Test Discord webhook on startup
  log.info('Testing Discord webhook...');
  sendDiscord('Server started and monitoring presence');

  // Test FCM notification on startup
  log.info('Testing FCM notification...');
  sendFCM('Server Started', 'Server started and monitoring presence');

  if (PUSHOVER_APP_TOKEN && PUSHOVER_USER_KEY) {
    log.info('Testing Pushover notification...');
    sendPushover('Server Started', 'Server started and monitoring presence');
  }

  if (TELEGRAM_BOT_TOKEN && TELEGRAM_CHAT_ID) {
    log.info('Testing Telegram notification...');
    sendTelegram('🤖 <b>Server Started</b>\nServer started and monitoring presence.');
  }

  watchPresence();
});

// ── Presence watcher ─────────────────────────────────────────────────────────
const knownStates = {};

function watchPresence() {
  db.ref('presence').on('value', (snapshot) => {
    const data = snapshot.val();
    if (!data || typeof data !== 'object') return;

    for (const [user, info] of Object.entries(data)) {
      if (!info || typeof info !== 'object') continue;

      const state = info.state || 'offline';
      const device = info.device || 'Unknown';
      const prev = knownStates[user];

      if (prev !== state) {
        knownStates[user] = state;
        const time = new Date().toLocaleTimeString();

        if (state === 'online') {
          log.event(`ONLINE     |   ${user.padEnd(18)} | Device: ${device} | Time: ${time}`);
          sendDiscord(`${device} is online | ${time}`);
          sendPushover('User Online', `${device} is online | ${time}`);
          sendTelegram(`🟢 <b>User Online</b>\n<b>${user}</b> is online on ${device} at ${time}`);

          // FCM notification to Android app
          sendFCM('User Online', `${device} is online | ${time}`);

          // Android push
          pushToAndroid({
            type: "PRESENCE_UPDATE",
            user,
            state,
            device,
            time
          });
          
          // ESP32 push
          pushToESP32({
            type: "PRESENCE_UPDATE",
            user,
            state,
            device,
            time
          });

        } else {
          log.event(`OFFLINE    |   ${user.padEnd(18)} | Time: ${time}`);
        }
      }
    }
  }, (err) => {
    log.error('Firebase error:', err.message);
  });

  log.system('Watching Firebase presence...');
}

// ── Heartbeat for ESP32 clients (keep connection alive) ─────────────────────
setInterval(() => {
  for (const ws of esp32Clients) {
    if (ws.readyState === 1) {
      ws.send(JSON.stringify({ type: 'PING' }));
    }
  }
}, 30000); // Every 30 seconds