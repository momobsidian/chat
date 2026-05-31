# ESP32 Wiring Guide

## 🔌 Components

1. **ESP32 Development Board** (any variant)
2. **OLED Display** - SSD1306, 128x64, I2C (4-pin)
3. **Buzzer** - Active or Passive (3-pin)

## 📐 Wiring Diagram

### OLED Display (4-pin I2C)

```
OLED Pin    →    ESP32 Pin
─────────────────────────────
VCC         →    3.3V
GND         →    GND
SCL         →    GPIO 22 (SCL)
SDA         →    GPIO 21 (SDA)
```

### Buzzer (3-pin)

```
Buzzer Pin  →    ESP32 Pin
─────────────────────────────
VCC         →    3.3V or 5V
GND         →    GND
Signal      →    GPIO 25
```

### Built-in LED (already on board)

```
LED         →    GPIO 2 (built-in)
```

## 🎨 Visual Diagram

```
                    ┌─────────────────┐
                    │                 │
                    │     ESP32       │
                    │                 │
    ┌───────────────┤ 3.3V            │
    │               │ GND             ├───────────┐
    │       ┌───────┤ GPIO 21 (SDA)   │           │
    │       │   ┌───┤ GPIO 22 (SCL)   │           │
    │       │   │   │ GPIO 25         ├─────┐     │
    │       │   │   │ GPIO 2 (LED)    │     │     │
    │       │   │   └─────────────────┘     │     │
    │       │   │                            │     │
    │       │   │   ┌──────────────┐         │     │
    │       │   │   │ OLED Display │         │     │
    │       │   │   │  128x64 I2C  │         │     │
    │       │   │   ├──────────────┤         │     │
    ├───────┼───┼───┤ VCC          │         │     │
    │       │   │   │ GND          ├─────────┼─────┤
    │       └───────┤ SDA          │         │     │
    │           └───┤ SCL          │         │     │
    │               └──────────────┘         │     │
    │                                        │     │
    │               ┌──────────────┐         │     │
    │               │   Buzzer     │         │     │
    │               │   (3-pin)    │         │     │
    │               ├──────────────┤         │     │
    └───────────────┤ VCC          │         │     │
                    │ GND          ├─────────┘     │
                    │ Signal       ├───────────────┘
                    └──────────────┘
```

## 📝 Pin Summary

| Component | Pin Name | ESP32 GPIO | Notes |
|-----------|----------|------------|-------|
| OLED | VCC | 3.3V | Power |
| OLED | GND | GND | Ground |
| OLED | SDA | GPIO 21 | I2C Data |
| OLED | SCL | GPIO 22 | I2C Clock |
| Buzzer | VCC | 3.3V or 5V | Power (5V for louder) |
| Buzzer | GND | GND | Ground |
| Buzzer | Signal | GPIO 25 | PWM Signal |
| LED | Built-in | GPIO 2 | On-board LED |

## ⚙️ Configuration Notes

### I2C Address
- Most SSD1306 OLED displays use address **0x3C**
- Some use **0x3D** - check your display
- To scan I2C address, use I2C Scanner sketch

### Buzzer Type
- **Active Buzzer**: Just needs HIGH/LOW signal
- **Passive Buzzer**: Needs PWM frequency (better for tones)
- Code supports both types

### Power Considerations
- OLED draws ~20mA
- Buzzer draws ~30mA (active) or ~10mA (passive)
- Total: ~50-60mA (well within ESP32 limits)
- Can power from USB or external 5V supply

## 🔧 Customizing Pins

If you need to use different pins, update these lines in the code:

```cpp
// Buzzer pin
#define BUZZER_PIN 25  // Change to your pin

// I2C pins (if not using default)
#define I2C_SDA 21
#define I2C_SCL 22

// LED pin
#define LED_PIN 2
```

## 🧪 Testing Components

### Test OLED Display
Upload this simple test:
```cpp
#include <Wire.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  Wire.begin(21, 22);
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED OK!");
    display.display();
  }
}

void loop() {}
```

### Test Buzzer
Upload this simple test:
```cpp
#define BUZZER_PIN 25

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  tone(BUZZER_PIN, 1000, 500);  // 1kHz for 500ms
  delay(1000);
}
```

## 🛠️ Troubleshooting

### OLED Not Working
- Check I2C address (try 0x3C or 0x3D)
- Verify wiring (SDA/SCL not swapped)
- Check power (3.3V, not 5V)
- Run I2C scanner to detect device

### Buzzer Not Working
- Check if active or passive type
- Verify GPIO 25 is not used by other peripherals
- Try different frequency (500-3000 Hz)
- Check power connection

### ESP32 Won't Boot
- Disconnect buzzer temporarily
- Check for short circuits
- Verify power supply is adequate
- Try different USB cable/port

## 📦 Shopping List

| Item | Quantity | Approx Price |
|------|----------|--------------|
| ESP32 Dev Board | 1 | $5-10 |
| OLED Display (SSD1306, I2C) | 1 | $3-5 |
| Buzzer (3-pin) | 1 | $1-2 |
| Jumper Wires | 10 | $1-2 |
| Breadboard (optional) | 1 | $2-3 |
| **Total** | | **$12-22** |

## 🎯 Ready to Upload!

Once wired:
1. Connect ESP32 via USB
2. Open `esp32-notification-client.ino`
3. Select board and port
4. Upload!

The display will show status and notifications will appear with sound!
