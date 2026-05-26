/**
 * ═══════════════════════════════════════════════════════════════════════════
 * 📡 NOTIFICATION SERVER HELPER
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * This file demonstrates how to implement a notification system using:
 * 1. Discord Webhooks (for web notifications)
 * 2. WebSocket (for real-time push to mobile apps)
 * 
 * You can copy this code into any Node.js web application!
 */

require('dotenv').config();
const http = require('http');
const WebSocket = require('ws');

// ═══════════════════════════════════════════════════════════════════════════
// 🔧 CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

const PORT = process.env.PORT || 3000;
const DISCORD_WEBHOOK = process.env.DISCORD_WEBHOOK; // Get from Discord channel settings

// ═══════════════════════════════════════════════════════════════════════════
// 📨 DISCORD NOTIFICATION FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Send notification to Discord channel via webhook
 * 
 * @param {string} message - The message to send to Discord
 * @returns {Promise<void>}
 * 
 * HOW TO GET DISCORD WEBHOOK:
 * 1. Go to your Discord server
 * 2. Right-click on a channel → Edit Channel
 * 3. Go to Integrations → Webhooks → New Webhook
 * 4. Copy the webhook URL
 * 5. Add to .env file: DISCORD_WEBHOOK=your_webhook_url_here
 */
async function sendDiscord(message) {
  // If no webhook configured, skip silently
  if (!DISCORD_WEBHOOK) {
    console.log('⚠️  Discord webhook not configured');
    return;
  }

  try {
    // Send POST request to Discord webhook
    const res = await fetch(DISCORD_WEBHOOK, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ content: message })
    });

    if (res.ok) {
      console.log(`✅ DISCORD SENT: ${message}`);
    } else {
      const errText = await res.text();
      console.error(`❌ DISCORD FAIL (${res.status}): ${errText}`);
    }
  } catch (err) {
    console.error('❌ DISCORD ERROR:', err.message);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// 📡 WEBSOCKET NOTIFICATION SYSTEM
// ═══════════════════════════════════════════════════════════════════════════

/**
 * WebSocket allows real-time push notifications to connected clients
 * (e.g., mobile apps, web browsers)
 * 
 * BENEFITS:
 * - Instant delivery (no polling needed)
 * - Low latency
 * - Bidirectional communication
 * - Efficient for mobile apps
 */

// Create HTTP server
const server = http.createServer((req, res) => {
  // Simple health check endpoint
  if (req.url === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ 
      status: 'ok', 
      connectedClients: androidClients.size,
      timestamp: new Date().toISOString()
    }));
    return;
  }

  // Example: Trigger notification via HTTP GET
  if (req.url === '/notify-test') {
    const testMessage = '🔔 Test notification from server!';
    
    // Send to Discord
    sendDiscord(testMessage);
    
    // Send to all connected WebSocket clients
    pushToAndroid({
      type: 'TEST_NOTIFICATION',
      message: testMessage,
      timestamp: new Date().toISOString()
    });

    res.writeHead(200, { 'Content-Type': 'text/html' });
    res.end('<h1>Notification sent!</h1><p>Check Discord and connected apps.</p>');
    return;
  }

  res.writeHead(404);
  res.end('Not Found');
});

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 WEBSOCKET SERVER SETUP
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create WebSocket server attached to HTTP server
 * This allows clients to connect via ws://your-server-ip:port
 */
const wss = new WebSocket.Server({ server });

/**
 * Store all connected clients
 * Using a Set for efficient add/remove operations
 */
const androidClients = new Set();

/**
 * Handle new WebSocket connections
 */
wss.on('connection', (ws, req) => {
  // Get client IP address
  const clientIp = req.socket.remoteAddress;
  
  console.log(`📱 New client connected from ${clientIp}`);
  console.log(`📊 Total connected clients: ${androidClients.size + 1}`);
  
  // Add client to our set
  androidClients.add(ws);

  // Send welcome message to the newly connected client
  ws.send(JSON.stringify({
    type: 'WELCOME',
    message: 'Connected to notification server',
    timestamp: new Date().toISOString()
  }));

  /**
   * Handle messages received from client
   * (Optional - for bidirectional communication)
   */
  ws.on('message', (data) => {
    try {
      const message = JSON.parse(data.toString());
      console.log('📩 Received from client:', message);
      
      // Example: Client can request their connection status
      if (message.type === 'PING') {
        ws.send(JSON.stringify({
          type: 'PONG',
          timestamp: new Date().toISOString()
        }));
      }
    } catch (err) {
      console.error('Error parsing client message:', err);
    }
  });

  /**
   * Handle client disconnection
   */
  ws.on('close', () => {
    androidClients.delete(ws);
    console.log(`📱 Client disconnected from ${clientIp}`);
    console.log(`📊 Total connected clients: ${androidClients.size}`);
  });

  /**
   * Handle WebSocket errors
   */
  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
    androidClients.delete(ws);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 📤 PUSH NOTIFICATION FUNCTION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Send notification to all connected WebSocket clients
 * 
 * @param {Object} data - The data to send to clients
 * 
 * USAGE EXAMPLES:
 * 
 * // Simple text notification
 * pushToAndroid({
 *   type: 'MESSAGE',
 *   text: 'New message received!'
 * });
 * 
 * // User presence update
 * pushToAndroid({
 *   type: 'PRESENCE_UPDATE',
 *   user: 'John Doe',
 *   state: 'online',
 *   timestamp: new Date().toISOString()
 * });
 * 
 * // Custom notification with action
 * pushToAndroid({
 *   type: 'ALERT',
 *   title: 'Important Update',
 *   body: 'Your order has been shipped!',
 *   action: 'VIEW_ORDER',
 *   orderId: '12345'
 * });
 */
function pushToAndroid(data) {
  // Convert data to JSON string
  const payload = JSON.stringify(data);
  
  // Track successful sends
  let successCount = 0;
  let failCount = 0;

  // Send to all connected clients
  for (const ws of androidClients) {
    // Check if connection is open (readyState === 1)
    if (ws.readyState === WebSocket.OPEN) {
      try {
        ws.send(payload);
        successCount++;
      } catch (err) {
        console.error('Failed to send to client:', err);
        failCount++;
        // Remove dead connection
        androidClients.delete(ws);
      }
    } else {
      // Remove closed connections
      androidClients.delete(ws);
      failCount++;
    }
  }

  console.log(`📤 Pushed to ${successCount} clients (${failCount} failed)`);
}

// ═══════════════════════════════════════════════════════════════════════════
// 🚀 START SERVER
// ═══════════════════════════════════════════════════════════════════════════

server.listen(PORT, '0.0.0.0', () => {
  console.log('');
  console.log('═══════════════════════════════════════════════════════');
  console.log('  📡 Notification Server Running!');
  console.log('═══════════════════════════════════════════════════════');
  console.log('');
  console.log(`  🌐 HTTP Server:  http://localhost:${PORT}`);
  console.log(`  📡 WebSocket:    ws://localhost:${PORT}`);
  console.log(`  🔔 Discord:      ${DISCORD_WEBHOOK ? 'Configured ✅' : 'Not configured ❌'}`);
  console.log('');
  console.log('  📝 Test endpoints:');
  console.log(`     Health check: http://localhost:${PORT}/health`);
  console.log(`     Test notify:  http://localhost:${PORT}/notify-test`);
  console.log('');
  console.log('  Press Ctrl+C to stop');
  console.log('═══════════════════════════════════════════════════════');
});

// ═══════════════════════════════════════════════════════════════════════════
// 🛠️ EXAMPLE USAGE IN YOUR APP
// ═══════════════════════════════════════════════════════════════════════════

/**
 * EXAMPLE 1: Send notification when user logs in
 */
function onUserLogin(user, deviceInfo) {
  const time = new Date().toLocaleTimeString('en-US', { hour12: true });
  
  // Send to Discord
  sendDiscord(
    `🔵 **USER LOGIN** 🔵\n` +
    `👤 User: **${user.name}** (@${user.username})\n` +
    `📱 Device: ${deviceInfo.model}\n` +
    `⏰ Time: ${time}\n` +
    `🌍 IP: ${deviceInfo.ip}`
  );

  // Send to mobile apps
  pushToAndroid({
    type: 'USER_LOGIN',
    user: user.name,
    username: user.username,
    device: deviceInfo.model,
    time: time,
    timestamp: Date.now()
  });
}

/**
 * EXAMPLE 2: Send notification when new message arrives
 */
function onNewMessage(sender, receiver, message) {
  // Send to Discord
  sendDiscord(
    `💬 **NEW MESSAGE** 💬\n` +
    `From: **${sender.name}**\n` +
    `To: **${receiver.name}**\n` +
    `Message: ${message.text.substring(0, 100)}...`
  );

  // Send to mobile apps
  pushToAndroid({
    type: 'NEW_MESSAGE',
    from: sender.name,
    to: receiver.name,
    message: message.text,
    timestamp: Date.now()
  });
}

/**
 * EXAMPLE 3: Send notification on important event
 */
function onImportantEvent(eventType, details) {
  // Send to Discord
  sendDiscord(
    `⚠️ **IMPORTANT EVENT** ⚠️\n` +
    `Type: ${eventType}\n` +
    `Details: ${details}`
  );

  // Send to mobile apps
  pushToAndroid({
    type: 'IMPORTANT_EVENT',
    eventType: eventType,
    details: details,
    timestamp: Date.now()
  });
}

// ═══════════════════════════════════════════════════════════════════════════
// 📱 CLIENT-SIDE WEBSOCKET CONNECTION (FOR REFERENCE)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * JAVASCRIPT CLIENT (Web Browser or Node.js):
 * 
 * const ws = new WebSocket('ws://your-server-ip:3000');
 * 
 * ws.onopen = () => {
 *   console.log('Connected to notification server');
 * };
 * 
 * ws.onmessage = (event) => {
 *   const data = JSON.parse(event.data);
 *   console.log('Notification received:', data);
 *   
 *   // Handle different notification types
 *   switch(data.type) {
 *     case 'USER_LOGIN':
 *       showNotification(`${data.user} logged in`);
 *       break;
 *     case 'NEW_MESSAGE':
 *       showNotification(`New message from ${data.from}`);
 *       break;
 *     // ... handle other types
 *   }
 * };
 * 
 * ws.onerror = (error) => {
 *   console.error('WebSocket error:', error);
 * };
 * 
 * ws.onclose = () => {
 *   console.log('Disconnected from notification server');
 *   // Optionally: attempt to reconnect
 * };
 */

/**
 * ANDROID CLIENT (Java/Kotlin):
 * 
 * // Add dependency: implementation 'org.java-websocket:Java-WebSocket:1.5.3'
 * 
 * WebSocketClient client = new WebSocketClient(new URI("ws://your-server-ip:3000")) {
 *     @Override
 *     public void onOpen(ServerHandshake handshake) {
 *         Log.d("WebSocket", "Connected");
 *     }
 * 
 *     @Override
 *     public void onMessage(String message) {
 *         JSONObject data = new JSONObject(message);
 *         String type = data.getString("type");
 *         
 *         // Show notification
 *         showNotification(data);
 *     }
 * 
 *     @Override
 *     public void onClose(int code, String reason, boolean remote) {
 *         Log.d("WebSocket", "Disconnected");
 *     }
 * 
 *     @Override
 *     public void onError(Exception ex) {
 *         Log.e("WebSocket", "Error", ex);
 *     }
 * };
 * 
 * client.connect();
 */

/**
 * iOS CLIENT (Swift):
 * 
 * import Starscream
 * 
 * let socket = WebSocket(url: URL(string: "ws://your-server-ip:3000")!)
 * 
 * socket.onEvent = { event in
 *     switch event {
 *     case .connected:
 *         print("Connected to notification server")
 *     case .text(let message):
 *         if let data = message.data(using: .utf8),
 *            let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any] {
 *             // Handle notification
 *             handleNotification(json)
 *         }
 *     case .disconnected(let reason, let code):
 *         print("Disconnected: \(reason) Code: \(code)")
 *     case .error(let error):
 *         print("Error: \(error?.localizedDescription ?? "")")
 *     default:
 *         break
 *     }
 * }
 * 
 * socket.connect()
 */

// ═══════════════════════════════════════════════════════════════════════════
// 🔒 GRACEFUL SHUTDOWN
// ═══════════════════════════════════════════════════════════════════════════

process.on('SIGINT', () => {
  console.log('\n\n👋 Shutting down notification server...\n');
  
  // Notify all clients about shutdown
  pushToAndroid({
    type: 'SERVER_SHUTDOWN',
    message: 'Server is shutting down',
    timestamp: Date.now()
  });

  // Close all WebSocket connections
  for (const ws of androidClients) {
    ws.close(1000, 'Server shutting down');
  }

  // Close HTTP server
  server.close(() => {
    console.log('✅ Server closed\n');
    process.exit(0);
  });
});

// ═══════════════════════════════════════════════════════════════════════════
// 📚 ADDITIONAL RESOURCES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * DISCORD WEBHOOK DOCUMENTATION:
 * https://discord.com/developers/docs/resources/webhook
 * 
 * WEBSOCKET DOCUMENTATION:
 * https://developer.mozilla.org/en-US/docs/Web/API/WebSocket
 * 
 * WS LIBRARY (Node.js):
 * https://github.com/websockets/ws
 * 
 * TIPS:
 * 1. Use environment variables for sensitive data (webhooks, tokens)
 * 2. Implement reconnection logic in clients
 * 3. Add authentication for WebSocket connections in production
 * 4. Consider using Redis for scaling across multiple servers
 * 5. Monitor connection count and server health
 * 6. Implement rate limiting to prevent spam
 * 7. Use SSL/TLS (wss://) in production for security
 */

// ═══════════════════════════════════════════════════════════════════════════
// 🎯 EXPORT FUNCTIONS (if using as module)
// ═══════════════════════════════════════════════════════════════════════════

module.exports = {
  sendDiscord,
  pushToAndroid,
  onUserLogin,
  onNewMessage,
  onImportantEvent
};
