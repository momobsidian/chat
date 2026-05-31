/*
 * ESP32 WebSocket Client for Online Notifications
 * Connects to: wss://saayem.qzz.io
 * 
 * Hardware: ESP32 (any variant)
 * Libraries needed:
 *   - WiFi (built-in)
 *   - WebSocketsClient by Markus Sattler
 *   - ArduinoJson by Benoit Blanchon
 */

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// ── WiFi Configuration ──────────────────────────────────────────────────────
const char* ssid = "YOUR_WIFI_SSID";           // Change this
const char* password = "YOUR_WIFI_PASSWORD";   // Change this

// ── WebSocket Configuration ─────────────────────────────────────────────────
const char* ws_host = "saayem.qzz.io";
const uint16_t ws_port = 443;  // HTTPS port for Cloudflare tunnel
const char* ws_path = "/esp32";  // ESP32-specific endpoint

WebSocketsClient webSocket;

// ── LED Configuration (optional) ────────────────────────────────────────────
#define LED_PIN 2  // Built-in LED on most ESP32 boards
bool ledState = false;

// ── Function Prototypes ─────────────────────────────────────────────────────
void connectWiFi();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handlePresenceUpdate(JsonDocument& doc);
void blinkLED(int times, int delayMs);

// ══════════════════════════════════════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32 Online Notification Client     ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup WebSocket with SSL
  webSocket.beginSSL(ws_host, ws_port, ws_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds if disconnected
  
  Serial.println("✓ WebSocket client initialized");
  Serial.printf("  Connecting to: wss://%s:%d%s\n\n", ws_host, ws_port, ws_path);
}

// ══════════════════════════════════════════════════════════════════════════════
// LOOP
// ══════════════════════════════════════════════════════════════════════════════
void loop() {
  webSocket.loop();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// WiFi Connection
// ══════════════════════════════════════════════════════════════════════════════
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s ", ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✓");
    Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  Signal: %d dBm\n", WiFi.RSSI());
    blinkLED(3, 200);  // Success blink
  } else {
    Serial.println(" ✗ Failed!");
    blinkLED(10, 100);  // Error blink
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// WebSocket Event Handler
// ══════════════════════════════════════════════════════════════════════════════
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("⚠ WebSocket Disconnected");
      digitalWrite(LED_PIN, LOW);
      break;
      
    case WStype_CONNECTED:
      Serial.println("✓ WebSocket Connected!");
      Serial.printf("  Server: wss://%s:%d%s\n", ws_host, ws_port, ws_path);
      
      // Send identification message
      webSocket.sendTXT("{\"type\":\"ESP32_HELLO\",\"device\":\"ESP32\"}");
      
      blinkLED(2, 300);
      break;
      
    case WStype_TEXT:
      Serial.printf("\n[RX] %s\n", payload);
      
      // Parse JSON
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.printf("✗ JSON parse error: %s\n", error.c_str());
        return;
      }
      
      // Handle different message types
      const char* msgType = doc["type"];
      
      if (strcmp(msgType, "PRESENCE_UPDATE") == 0) {
        handlePresenceUpdate(doc);
      } else if (strcmp(msgType, "PING") == 0) {
        webSocket.sendTXT("{\"type\":\"PONG\"}");
      }
      break;
      
    case WStype_ERROR:
      Serial.printf("✗ WebSocket Error: %s\n", payload);
      break;
      
    case WStype_PING:
      Serial.println("← PING");
      break;
      
    case WStype_PONG:
      Serial.println("→ PONG");
      break;
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// Handle Presence Update
// ══════════════════════════════════════════════════════════════════════════════
void handlePresenceUpdate(JsonDocument& doc) {
  const char* user = doc["user"];
  const char* state = doc["state"];
  const char* device = doc["device"];
  const char* time = doc["time"];
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       PRESENCE UPDATE                  ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.printf("║ User:   %-30s ║\n", user);
  Serial.printf("║ State:  %-30s ║\n", state);
  Serial.printf("║ Device: %-30s ║\n", device);
  Serial.printf("║ Time:   %-30s ║\n", time);
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Visual feedback
  if (strcmp(state, "online") == 0) {
    // User came online - fast blinks
    blinkLED(5, 100);
    
    // You can add more actions here:
    // - Turn on a relay
    // - Send to another device
    // - Display on OLED/LCD
    // - Play a sound
    // - etc.
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// LED Blink Helper
// ══════════════════════════════════════════════════════════════════════════════
void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}
