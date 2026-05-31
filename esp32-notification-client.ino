/*
 * ESP32 WebSocket Client for Online Notifications
 * Connects to: wss://saayem.qzz.io OR Render hosted URL
 * 
 * Hardware: ESP32 (any variant)
 * Components:
 *   - OLED Display (SSD1306, 128x64, I2C, 4-pin: VCC, GND, SCL, SDA)
 *   - Buzzer (3-pin: VCC, GND, Signal)
 * 
 * Libraries needed:
 *   - WiFi (built-in)
 *   - WebSocketsClient by Markus Sattler
 *   - ArduinoJson by Benoit Blanchon
 *   - Adafruit SSD1306 by Adafruit
 *   - Adafruit GFX Library by Adafruit
 */

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── WiFi Configuration ──────────────────────────────────────────────────────
const char* ssid = "Sayem";
const char* password = "Sayem123";

// ── WebSocket Configuration ─────────────────────────────────────────────────
// Primary server (Cloudflare tunnel)
const char* ws_host_primary = "saayem.qzz.io";
const uint16_t ws_port_primary = 443;

// Fallback server (Render hosted - update with your Render URL)
const char* ws_host_fallback = "your-app.onrender.com";  // Change this to your Render URL
const uint16_t ws_port_fallback = 443;

const char* ws_path = "/esp32";  // ESP32-specific endpoint

// Current server selection
bool usePrimaryServer = true;
const char* current_host;
uint16_t current_port;

WebSocketsClient webSocket;

// ── OLED Display Configuration (I2C) ───────────────────────────────────────
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Reset pin (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  // I2C address (usually 0x3C or 0x3D)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── Buzzer Configuration ────────────────────────────────────────────────────
#define BUZZER_PIN 25  // Change to your buzzer signal pin (GPIO 25)

// ── LED Configuration (optional) ────────────────────────────────────────────
#define LED_PIN 2  // Built-in LED on most ESP32 boards

// ── Pin Definitions for I2C (if needed to specify) ─────────────────────────
#define I2C_SDA 21  // Default SDA pin
#define I2C_SCL 22  // Default SCL pin

// ── State Variables ─────────────────────────────────────────────────────────
bool ledState = false;
unsigned long lastNotificationTime = 0;
String lastUser = "";
String lastDevice = "";

// ── Function Prototypes ─────────────────────────────────────────────────────
void connectWiFi();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handlePresenceUpdate(JsonDocument& doc);
void blinkLED(int times, int delayMs);
void playBuzzer(int frequency, int duration);
void playNotificationTone();
void updateDisplay(const char* line1, const char* line2 = "", const char* line3 = "", const char* line4 = "");
void displayNotification(const char* user, const char* device, const char* time);
void displayStatus(const char* status);
void switchServer();

// ══════════════════════════════════════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32 Online Notification Client     ║");
  Serial.println("║  with OLED Display & Buzzer            ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Setup Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("✗ SSD1306 allocation failed!");
    // Continue anyway, just won't have display
  } else {
    Serial.println("✓ OLED Display initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 Notification");
    display.println("Client");
    display.println("");
    display.println("Initializing...");
    display.display();
  }
  
  // Test buzzer
  playBuzzer(1000, 100);
  delay(100);
  playBuzzer(1500, 100);
  
  // Connect to WiFi
  displayStatus("Connecting WiFi");
  connectWiFi();
  
  // Setup WebSocket with SSL
  current_host = ws_host_primary;
  current_port = ws_port_primary;
  
  webSocket.beginSSL(current_host, current_port, ws_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds if disconnected
  
  Serial.println("✓ WebSocket client initialized");
  Serial.printf("  Connecting to: wss://%s:%d%s\n\n", current_host, current_port, ws_path);
  
  displayStatus("Connecting...");
}

// ══════════════════════════════════════════════════════════════════════════════
// LOOP
// ══════════════════════════════════════════════════════════════════════════════
void loop() {
  webSocket.loop();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi disconnected! Reconnecting...");
    displayStatus("WiFi Lost!");
    connectWiFi();
  }
  
  // Auto-clear display after 30 seconds of last notification
  if (lastNotificationTime > 0 && millis() - lastNotificationTime > 30000) {
    displayStatus("Waiting...");
    lastNotificationTime = 0;
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
    
    displayStatus("WiFi Connected!");
    blinkLED(3, 200);  // Success blink
    playBuzzer(2000, 200);  // Success tone
  } else {
    Serial.println(" ✗ Failed!");
    displayStatus("WiFi Failed!");
    blinkLED(10, 100);  // Error blink
    playBuzzer(500, 500);  // Error tone
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
      displayStatus("Disconnected!");
      
      // Try fallback server after 3 failed attempts
      if (usePrimaryServer) {
        Serial.println("Trying fallback server...");
        switchServer();
      }
      break;
      
    case WStype_CONNECTED:
      Serial.println("✓ WebSocket Connected!");
      Serial.printf("  Server: wss://%s:%d%s\n", current_host, current_port, ws_path);
      
      displayStatus("Connected!");
      
      // Send identification message
      webSocket.sendTXT("{\"type\":\"ESP32_HELLO\",\"device\":\"ESP32-OLED\"}");
      
      blinkLED(2, 300);
      playBuzzer(1500, 150);
      delay(100);
      playBuzzer(2000, 150);
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
      } else if (strcmp(msgType, "WELCOME") == 0) {
        Serial.println("✓ Server welcomed ESP32");
        displayStatus("Ready!");
      }
      break;
      
    case WStype_ERROR:
      Serial.printf("✗ WebSocket Error: %s\n", payload);
      displayStatus("Error!");
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
  
  // Visual and audio feedback
  if (strcmp(state, "online") == 0) {
    // Update display
    displayNotification(user, device, time);
    
    // Play notification tone
    playNotificationTone();
    
    // Blink LED
    blinkLED(5, 100);
    
    // Store last notification time
    lastNotificationTime = millis();
    lastUser = String(user);
    lastDevice = String(device);
  }
}

// ══════════════════════════════════════════════════════════════════════════════
// Display Functions
// ══════════════════════════════════════════════════════════════════════════════
void updateDisplay(const char* line1, const char* line2, const char* line3, const char* line4) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println(line1);
  
  if (strlen(line2) > 0) {
    display.setCursor(0, 16);
    display.println(line2);
  }
  
  if (strlen(line3) > 0) {
    display.setCursor(0, 32);
    display.println(line3);
  }
  
  if (strlen(line4) > 0) {
    display.setCursor(0, 48);
    display.println(line4);
  }
  
  display.display();
}

void displayNotification(const char* user, const char* device, const char* time) {
  display.clearDisplay();
  
  // Title
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ONLINE!");
  
  // User (larger text)
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println(user);
  
  // Device
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Device: ");
  display.println(device);
  
  // Time
  display.setCursor(0, 52);
  display.print("Time: ");
  display.println(time);
  
  display.display();
}

void displayStatus(const char* status) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Notification");
  display.println("Client");
  display.println("");
  display.setTextSize(2);
  display.println(status);
  display.display();
}

// ══════════════════════════════════════════════════════════════════════════════
// Buzzer Functions
// ══════════════════════════════════════════════════════════════════════════════
void playBuzzer(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

void playNotificationTone() {
  // Pleasant notification melody
  playBuzzer(1000, 100);
  delay(50);
  playBuzzer(1500, 100);
  delay(50);
  playBuzzer(2000, 150);
  delay(100);
  playBuzzer(1500, 200);
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

// ══════════════════════════════════════════════════════════════════════════════
// Server Switching (Primary/Fallback)
// ══════════════════════════════════════════════════════════════════════════════
void switchServer() {
  usePrimaryServer = !usePrimaryServer;
  
  if (usePrimaryServer) {
    current_host = ws_host_primary;
    current_port = ws_port_primary;
    Serial.println("Switching to primary server (saayem.qzz.io)");
  } else {
    current_host = ws_host_fallback;
    current_port = ws_port_fallback;
    Serial.println("Switching to fallback server (Render)");
  }
  
  webSocket.disconnect();
  delay(1000);
  webSocket.beginSSL(current_host, current_port, ws_path);
}

