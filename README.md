# ESP32 ARGB Fan Controller

ESP32‑based Wi‑Fi RGB controller for 6‑pin ARGB PC fans (Aigo AR12) with web UI, 20 effects, 10 color themes, OLED display and EEPROM settings storage.

<img width="2560" height="1920" alt="image" src="https://github.com/user-attachments/assets/284db8af-17c8-4963-be66-5b3fd2b8cf93" />

## Features

- Full replacement for dead Aigo AR12-2012 ARGB hub
- Control addressable LEDs inside 6-pin ARGB fans (WS2812/SM16703 compatible)
- 20 animation modes (Static, Breathing, Rainbow, Pulse, Fire, Thunder, Candle, etc.)
- 10 color themes (presets) with quick switching
- Wi‑Fi web UI (HTML+CSS+JS hosted on ESP32)
- REST API for integration with Home Assistant / scripts
- Settings stored in EEPROM (mode, theme, brightness, speed, color)
- Optional SSD1306 OLED display: shows IP, current mode, theme and brightness

## Hardware

- ESP32 DevKit V1 (or any ESP32 board with Wi‑Fi)
- 6‑pin ARGB fans Aigo AR12 (or similar with WS2812-like LEDs)
- Optional: original 6‑pin AR12-2012 hub (can be used as a simple signal splitter)
- 330–470 Ω resistor (data line protection)
- Optional: 1000 µF 6.3V+ capacitor on 5V line (for LED power stability)
- SSD1306 0.96" I2C OLED (optional but recommended)
- Dupont wires / soldering tools

### 6‑pin Aigo AR12 pinout

Looking at the fan connector (wire side):

1. +12V  — motor power (do not connect to ESP32)
2. GND   — motor ground
3. GND   — LED ground
4. Data IN  — ARGB data input (WS2812/SM16703)
5. +5V  — LED power
6. Data OUT — data output to next fan

## Wiring

Minimal setup (one or more fans chained):

- ESP32 GPIO 5 → 330 Ω resistor → Fan Pin 4 (Data IN)
- ESP32 GND → Fan Pin 3 (LED GND)
- +5V from PSU (or DC-DC) → Fan Pin 5 (+5V LED)
- Fan Pin 6 (Data OUT) → Pin 4 (Data IN) of the next fan → …

<img width="2560" height="1920" alt="image" src="https://github.com/user-attachments/assets/21daaae9-38a6-4f10-abe5-cc672e7f314f" />


Motors are powered separately by 12V from the PC PSU (SATA/Molex). Only LED part is controlled by ESP32.

### OLED

- ESP32 GPIO 21 → SDA (SSD1306)
- ESP32 GPIO 22 → SCL (SSD1306)
- ESP32 3.3V → VCC
- ESP32 GND → GND

## Firmware

Project is written using Arduino framework for ESP32.

Main components:

- `WiFi.h` + `WebServer.h` — HTTP server and Wi‑Fi connection
- `Adafruit_NeoPixel` — driving addressable LEDs
- `EEPROM.h` — non-volatile settings storage
- `Adafruit_SSD1306` + `Adafruit_GFX` — OLED UI (optional)

Key configuration constants in the sketch:

```cpp
#define RGB_PIN   5      // ESP32 GPIO for LED data
#define NUM_LEDS  8      // Number of LEDs per fan
#define NUM_FANS  5      // Number of fans in the chain
```

Total LED count = `NUM_LEDS * NUM_FANS`.

Wi‑Fi credentials:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### Building and flashing

1. Install **Arduino IDE 2.x**
2. Install **ESP32 core** via Boards Manager (Espressif Systems)
3. Install libraries via **Sketch → Include Library → Manage Libraries**:
   - Adafruit NeoPixel
   - Adafruit GFX Library
   - Adafruit SSD1306
4. Select board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
5. Select correct **Port (COMx / /dev/ttyUSBx)**
6. Paste the sketch from this repository, adjust `ssid`, `password`, `NUM_FANS` if needed
7. Press **Upload**

Open **Serial Monitor** at 115200 baud and wait for a line like:

```text
WiFi connected!
IP: 192.168.x.x
Web Server started!
```

Use this IP in your browser.

## Web UI

The ESP32 serves a modern single-page web UI:

- 20 mode buttons (Static, Breathing, Rainbow, Pulse, etc.)
- 10 theme buttons (color presets)
- Brightness slider (0–255)
- Speed slider (10–100)
- RGB color picker + numeric RGB inputs
- Status block with current mode/theme and connection state

Open in any modern browser:

```text
http://192.168.x.x
```

(Replace with the IP shown in Serial Monitor / OLED.)

## REST API

All controls are available via simple HTTP POST requests (form-encoded):

- `POST /api/mode` — change animation mode
  - body: `value=0..19`
- `POST /api/theme` — change color theme
  - body: `value=0..9`
- `POST /api/brightness` — change brightness
  - body: `value=0..255`
- `POST /api/speed` — change animation speed
  - body: `value=10..100`
- `POST /api/color` — set custom RGB color
  - body: `r=0..255&g=0..255&b=0..255`

Examples (curl):

```bash
curl -X POST "http://192.168.x.x/api/mode"       -d "value=2"     # Rainbow
curl -X POST "http://192.168.x.x/api/brightness" -d "value=128"   # 50% brightness
curl -X POST "http://192.168.x.x/api/color"      -d "r=255&g=0&b=0" # Red
```

All changes are saved to EEPROM automatically.

## Safety notes

- Do **not** connect 12V to ESP32 or LED +5V pin
- Use a resistor (330–470 Ω) in series with the data line
- Use a separate 5V rail with enough current for all LEDs
- If powering ESP32 from the same PSU as the PC, make sure grounds are common

## Roadmap / TODO

- [ ] Home Assistant auto-discovery config examples
- [ ] Optional MQTT interface
- [ ] Music-reactive modes using microphone (MAX9814 / INMP441)
- [ ] Temperature-based color mode (CPU/GPU sensors from PC)
- [ ] 3D-printable enclosure for the controller

