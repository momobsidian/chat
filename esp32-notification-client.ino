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
#include <WebSocketsClient_Generic.h>
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
#define BUZZER_CHANNEL 0  // PWM channel for buzzer
#define BUZZER_RESOLUTION 8  // 8-bit resolution

// ── LED Configuration (optional) ────────────────────────────────────────────
#define LED_PIN 2  // Built-in LED on most ESP32 boards

// ── Pin Definitions for I2C (if needed to specify) ─────────────────────────
#define I2C_SDA 21  // Default SDA pin
#define I2C_SCL 22  // Default SCL pin

// ── State Variables ─────────────────────────────────────────────────────────
bool ledState = false;
unsigned long lastNotificationTime = 0;
unsigned long lastPingTime = 0;
unsigned long lastPongTime = 0;
bool isConnected = false;
String lastUser = "";
String lastDevice = "";

// ── Function Prototypes ─────────────────────────────────────────────────────
void connectWiFi();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void handlePresenceUpdate(JsonDocument& doc);
void handleLoginVisit(JsonDocument& doc);
void blinkLED(int times, int delayMs);
void playBuzzer(int frequency, int duration);
void playNotificationTone();
void playLoginTone();
void updateDisplay(const char* line1, const char* line2 = "", const char* line3 = "", const char* line4 = "");
void displayLoginVisit(const char* device, const char* time);
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
  
  // Setup Buzzer with PWM (ESP32 v3.x API)
  ledcAttach(BUZZER_PIN, 2000, BUZZER_RESOLUTION);
  ledcWrite(BUZZER_PIN, 0);  // Start silent
  
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
  
  // Check WebSocket connection health (heartbeat)
  unsigned long now = millis();
  
  // If connected, send PING every 15 seconds
  if (isConnected && (now - lastPingTime > 15000)) {
    webSocket.sendTXT("{\"type\":\"PONG\"}");  // Send PONG as heartbeat
    lastPingTime = now;
    Serial.println("→ Heartbeat sent");
  }
  
  // If no PONG received for 45 seconds, assume disconnected
  if (isConnected && (now - lastPongTime > 45000)) {
    Serial.println("⚠ No heartbeat response - reconnecting...");
    isConnected = false;
    webSocket.disconnect();
    delay(1000);
    webSocket.beginSSL(current_host, current_port, ws_path);
    displayStatus("Reconnecting...");
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
      isConnected = false;
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
      
      isConnected = true;
      lastPongTime = millis();  // Reset heartbeat timer
      lastPingTime = millis();
      
      displayStatus("Connected!");
      
      // Send identification message
      webSocket.sendTXT("{\"type\":\"ESP32_HELLO\",\"device\":\"ESP32-OLED\"}");
      
      blinkLED(2, 300);
      playBuzzer(1500, 150);
      delay(100);
      playBuzzer(2000, 150);
      break;
      
    case WStype_TEXT:
      {
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
        } else if (strcmp(msgType, "LOGIN_VISIT") == 0) {
          handleLoginVisit(doc);
        } else if (strcmp(msgType, "PING") == 0) {
          webSocket.sendTXT("{\"type\":\"PONG\"}");
          lastPongTime = millis();  // Update heartbeat timer
          Serial.println("← PING (responded with PONG)");
        } else if (strcmp(msgType, "WELCOME") == 0) {
          Serial.println("✓ Server welcomed ESP32");
          displayStatus("Ready!");
          lastPongTime = millis();  // Update heartbeat timer
        }
      }
      break;
      
    case WStype_ERROR:
      Serial.printf("✗ WebSocket Error: %s\n", payload);
      displayStatus("Error!");
      break;
      
    case WStype_PING:
      Serial.println("← WebSocket PING");
      lastPongTime = millis();  // Update heartbeat timer
      break;
      
    case WStype_PONG:
      Serial.println("→ WebSocket PONG");
      lastPongTime = millis();  // Update heartbeat timer
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
// Handle Login Visit
// ══════════════════════════════════════════════════════════════════════════════
void handleLoginVisit(JsonDocument& doc) {
  const char* device = doc["device"];
  const char* time = doc["time"];
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       LOGIN PAGE VISIT                 ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.printf("║ Device: %-30s ║\n", device);
  Serial.printf("║ Time:   %-30s ║\n", time);
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Visual and audio feedback
  displayLoginVisit(device, time);
  
  // Play different tone for login (more urgent)
  playLoginTone();
  
  // Blink LED rapidly
  blinkLED(7, 80);
  
  // Store last notification time
  lastNotificationTime = millis();
}

// ══════════════════════════════════════════════════════════════════════════════
// Display Functions  ── PREMIUM REDESIGN (visual only, no logic changes)
// ══════════════════════════════════════════════════════════════════════════════

// ── Helper: draw a thin horizontal rule ─────────────────────────────────────
static void drawRule(int y) {
  display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_WHITE);
}

// ── Helper: draw a rounded rectangle border ─────────────────────────────────
static void drawRoundBorder() {
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);
}

// ── Helper: draw signal bars (3 bars, filled count = strength 0-3) ──────────
static void drawSignalBars(int x, int y, int filled) {
  // Three ascending bars: 3px, 5px, 7px tall, each 3px wide, 2px apart
  for (int i = 0; i < 3; i++) {
    int bw = 3, gap = 5;
    int bh = 3 + i * 2;           // heights: 3, 5, 7
    int bx = x + i * gap;
    int by = y + (7 - bh);        // align bottoms
    if (i < filled) {
      display.fillRect(bx, by, bw, bh, SSD1306_WHITE);
    } else {
      display.drawRect(bx, by, bw, bh, SSD1306_WHITE);
    }
  }
}

// ── Helper: animated progress bar (call inside a loop before display()) ─────
// Draws a progress bar at y=54, width proportional to pct (0–100)
static void drawProgressBar(int pct) {
  int barX = 4, barY = 54, barW = 120, barH = 6;
  display.drawRoundRect(barX, barY, barW, barH, 3, SSD1306_WHITE);
  int fill = (barW - 2) * pct / 100;
  if (fill > 0) {
    display.fillRoundRect(barX + 1, barY + 1, fill, barH - 2, 2, SSD1306_WHITE);
  }
}

// ── updateDisplay() ──────────────────────────────────────────────────────────
// Clean 4-line layout with top rule and consistent spacing.
// Signature unchanged — drop-in replacement.
void updateDisplay(const char* line1, const char* line2, const char* line3, const char* line4) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Top accent rule
  drawRule(9);

  // Line 1 — header / title area (above rule)
  display.setCursor(2, 1);
  display.print(line1);

  // Lines 2-4 — body content below rule
  if (strlen(line2) > 0) {
    display.setCursor(2, 14);
    display.println(line2);
  }
  if (strlen(line3) > 0) {
    display.setCursor(2, 28);
    display.println(line3);
  }
  if (strlen(line4) > 0) {
    display.setCursor(2, 42);
    display.println(line4);
  }

  drawRoundBorder();
  display.display();
}

// ── displayNotification() ───────────────────────────────────────────────────
// Premium notification card with:
//   - Inverted "ONLINE" header bar
//   - Large username (textSize 2)
//   - Device and time rows with label prefixes
//   - Flash animation (3 quick border blinks to grab attention)
// Signature unchanged — drop-in replacement.
void displayNotification(const char* user, const char* device, const char* time) {

  // Flash animation: invert screen 3× before settling on the card
  for (int flash = 0; flash < 3; flash++) {
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(60);
    display.clearDisplay();
    display.display();
    delay(60);
  }

  // ── Build the notification card ────────────────────────────────────────
  display.clearDisplay();

  // Inverted header bar (y=0..13): filled rectangle with "ONLINE" in black
  display.fillRect(0, 0, SCREEN_WIDTH, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(3, 3);
  display.print("* ONLINE *");

  // Signal indicator in header bar (top-right)
  drawSignalBars(109, 3, 3);   // 3 filled = strong

  // Switch back to white text for body
  display.setTextColor(SSD1306_WHITE);

  // Username — large text (textSize 2 = 12px tall glyphs)
  display.setTextSize(2);
  display.setCursor(2, 17);
  // Truncate if longer than ~8 chars to avoid overflow at textSize 2
  String userName = String(user);
  if (userName.length() > 8) {
    userName = userName.substring(0, 7) + ".";
  }
  display.print(userName);

  // Divider below username
  drawRule(35);

  // Device row (textSize 1)
  display.setTextSize(1);
  display.setCursor(2, 39);
  display.print("Dev: ");
  // Truncate device to fit remaining width (~15 chars after label)
  String devStr = String(device);
  if (devStr.length() > 14) {
    devStr = devStr.substring(0, 13) + ".";
  }
  display.print(devStr);

  // Time row
  display.setCursor(2, 51);
  display.print("At:  ");
  String timeStr = String(time);
  if (timeStr.length() > 14) {
    timeStr = timeStr.substring(0, 13) + ".";
  }
  display.print(timeStr);

  // Outer rounded border
  drawRoundBorder();

  display.display();
}

// ── displayLoginVisit() ─────────────────────────────────────────────────────
// Premium login visit alert with:
//   - Red inverted "🚨 LOGIN ALERT" header bar
//   - Large "VISITOR" text
//   - Device and time information
//   - More urgent flash animation (5 quick blinks)
void displayLoginVisit(const char* device, const char* time) {

  // Flash animation: more urgent (5 blinks)
  for (int flash = 0; flash < 5; flash++) {
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    delay(50);
    display.clearDisplay();
    display.display();
    delay(50);
  }

  // ── Build the login alert card ─────────────────────────────────────────
  display.clearDisplay();

  // Inverted header bar (y=0..13): filled rectangle with alert text
  display.fillRect(0, 0, SCREEN_WIDTH, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(3, 3);
  display.print("! LOGIN ALERT !");

  // Warning indicator in header bar (top-right) - all bars filled
  drawSignalBars(109, 3, 3);

  // Switch back to white text for body
  display.setTextColor(SSD1306_WHITE);

  // "VISITOR" text — large
  display.setTextSize(2);
  display.setCursor(2, 17);
  display.print("VISITOR");

  // Divider below title
  drawRule(35);

  // Device row (textSize 1)
  display.setTextSize(1);
  display.setCursor(2, 39);
  display.print("Dev: ");
  String devStr = String(device);
  if (devStr.length() > 14) {
    devStr = devStr.substring(0, 13) + ".";
  }
  display.print(devStr);

  // Time row
  display.setCursor(2, 51);
  display.print("At:  ");
  String timeStr = String(time);
  if (timeStr.length() > 14) {
    timeStr = timeStr.substring(0, 13) + ".";
  }
  display.print(timeStr);

  // Outer rounded border
  drawRoundBorder();

  display.display();
}

// ── displayStatus() ─────────────────────────────────────────────────────────
// Premium status screen with:
//   - "ESP32" branding bar at top (inverted)
//   - Centred status text (textSize 1 for short, textSize 1 for all)
//   - Animated progress bar that sweeps across the bottom
//   - Context-aware signal bars for WiFi states
// Signature unchanged — drop-in replacement.
void displayStatus(const char* status) {

  // ── Animated progress bar sweep (non-blocking feel: 5 quick frames) ────
  // Only animate for "connecting" type messages to avoid delays elsewhere
  String s = String(status);
  bool doAnimate = (s.indexOf("Connect") >= 0 || s.indexOf("connect") >= 0
                    || s.indexOf("Waiting") >= 0 || s.indexOf("Init") >= 0);

  if (doAnimate) {
    for (int pct = 0; pct <= 100; pct += 25) {
      display.clearDisplay();

      // Inverted branding bar
      display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setTextSize(1);
      display.setCursor(3, 2);
      display.print("ESP32  NOTIFY");

      // Status text (white, centred vertically in remaining space)
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);

      // Centre horizontally: each char is ~6px wide at textSize 1
      int textW = strlen(status) * 6;
      int cx = (SCREEN_WIDTH - textW) / 2;
      if (cx < 2) cx = 2;
      display.setCursor(cx, 26);
      display.print(status);

      // Ellipsis dots cycling with pct
      int dots = (pct / 25) % 4;
      display.setCursor(cx + textW + 2, 26);
      for (int d = 0; d < dots; d++) display.print(".");

      // Progress bar
      drawProgressBar(pct);

      drawRoundBorder();
      display.display();
      delay(60);
    }
  }

  // ── Final static frame ─────────────────────────────────────────────────
  display.clearDisplay();

  // Inverted branding bar
  display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(3, 2);
  display.print("ESP32  NOTIFY");

  // Signal bars in header — context aware
  int signalFill = 0;
  if (s.indexOf("Connected") >= 0 || s.indexOf("Ready") >= 0) signalFill = 3;
  else if (s.indexOf("WiFi") >= 0 && s.indexOf("Failed") < 0)  signalFill = 2;
  else if (s.indexOf("Connecting") >= 0)                        signalFill = 1;
  drawSignalBars(109, 2, signalFill);

  // Status text
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  int textW = strlen(status) * 6;
  int cx = (SCREEN_WIDTH - textW) / 2;
  if (cx < 2) cx = 2;
  display.setCursor(cx, 26);
  display.print(status);

  // Solid progress bar at 100% for final frame
  drawProgressBar(100);

  drawRoundBorder();
  display.display();
}

// ══════════════════════════════════════════════════════════════════════════════
// Buzzer Functions (PWM-based for clean sound)
// ══════════════════════════════════════════════════════════════════════════════
void playBuzzer(int frequency, int duration) {
  if (frequency > 0) {
    ledcAttach(BUZZER_PIN, frequency, BUZZER_RESOLUTION);
    ledcWrite(BUZZER_PIN, 128);  // 50% duty cycle for clean tone
    delay(duration);
    ledcWrite(BUZZER_PIN, 0);  // Stop
  } else {
    delay(duration);
  }
}

void playNotificationTone() {
  // Clean notification melody with proper frequencies
  playBuzzer(523, 120);   // C5
  delay(30);
  playBuzzer(659, 120);   // E5
  delay(30);
  playBuzzer(784, 150);   // G5
  delay(50);
  playBuzzer(659, 180);   // E5
  ledcWrite(BUZZER_PIN, 0);  // Ensure silence at end
}

void playLoginTone() {
  // More urgent alert tone for login visits (higher pitch, faster)
  playBuzzer(880, 100);   // A5
  delay(20);
  playBuzzer(988, 100);   // B5
  delay(20);
  playBuzzer(1047, 120);  // C6
  delay(30);
  playBuzzer(988, 100);   // B5
  delay(20);
  playBuzzer(880, 150);   // A5
  ledcWrite(BUZZER_PIN, 0);  // Ensure silence at end
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