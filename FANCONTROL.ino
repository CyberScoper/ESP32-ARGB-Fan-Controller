#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <algorithm>

// ===== EEPROM АДРЕСА =====
#define EEPROM_SIZE 512
#define ADDR_MODE 0
#define ADDR_BRIGHTNESS 1
#define ADDR_R 2
#define ADDR_G 3
#define ADDR_B 4
#define ADDR_SPEED 5
#define ADDR_THEME 6

// ===== НАСТРОЙКИ WIFI =====
const char* ssid = "ssid";
const char* password = "passwd";

// ===== НАСТРОЙКИ GPIO =====
#define RGB_PIN 5
#define OLED_SDA 21
#define OLED_SCL 22

// ===== НАСТРОЙКИ OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== НАСТРОЙКИ RGB =====
#define NUM_LEDS 8
#define NUM_FANS 5

Adafruit_NeoPixel strip(NUM_LEDS * NUM_FANS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ===== ПЕРЕМЕННЫЕ СОСТОЯНИЯ =====
int currentMode = 0;
int brightness = 255;
int r = 255, g = 0, b = 0;
int speed = 50;
int currentTheme = 0;

// Структура для хранения цветовой схемы
struct ColorScheme {
  const char* name;
  int r, g, b;
};

// 10 ТЕМАТИЧЕСКИХ ЦВЕТОВЫХ СХЕМ
ColorScheme themes[10] = {
  {"Красный", 255, 0, 0},
  {"Синий", 0, 100, 255},
  {"Зелёный", 0, 255, 0},
  {"Фиолетовый", 200, 0, 200},
  {"Оранжевый", 255, 165, 0},
  {"Голубой", 0, 255, 255},
  {"Розовый", 255, 20, 147},
  {"Жёлтый", 255, 255, 0},
  {"Лайм", 50, 205, 50},
  {"Белый", 255, 255, 255}
};

// МАССИВ ИМЕН РЕЖИМОВ - ГЛОБАЛЬНЫЙ!
const char* modeNames[] = {
  "Static", "Breathing", "Rainbow", "Pulse", "Off",
  "Strobe", "Wave", "Fade", "Twinkle", "Fire",
  "Waterfall", "Scanline", "Bounce", "Chase", "Sparkle",
  "PulseRainbow", "Cyclone", "Metallic", "Thunder", "Candle"
};

WebServer server(80);

// ===== ФУНКЦИИ EEPROM =====

void loadSettings() {
  EEPROM.begin(EEPROM_SIZE);
  currentMode = EEPROM.read(ADDR_MODE) % 20;
  brightness = EEPROM.read(ADDR_BRIGHTNESS);
  r = EEPROM.read(ADDR_R);
  g = EEPROM.read(ADDR_G);
  b = EEPROM.read(ADDR_B);
  speed = EEPROM.read(ADDR_SPEED);
  currentTheme = EEPROM.read(ADDR_THEME) % 10;
  
  Serial.println("Settings loaded from EEPROM");
  Serial.print("Mode: "); Serial.println(currentMode);
  Serial.print("RGB: "); Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.println(b);
}

void saveSettings() {
  EEPROM.write(ADDR_MODE, currentMode);
  EEPROM.write(ADDR_BRIGHTNESS, brightness);
  EEPROM.write(ADDR_R, r);
  EEPROM.write(ADDR_G, g);
  EEPROM.write(ADDR_B, b);
  EEPROM.write(ADDR_SPEED, speed);
  EEPROM.write(ADDR_THEME, currentTheme);
  EEPROM.commit();
  
  Serial.println("Settings saved to EEPROM!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32 RGB Controller with THEMES ===");
  
  loadSettings();
  
  Serial.println("Initializing OLED...");
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: OLED not found!");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("RGB Controller");
  display.println("Connecting WiFi...");
  display.display();
  
  Serial.println("Initializing RGB LEDs...");
  strip.begin();
  strip.show();
  
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi OK!");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
  } else {
    Serial.println("\nWiFi FAILED!");
  }
  
  Serial.println("Setting up Web Server...");
  setupWebServer();
  server.begin();
  Serial.println("Web Server started!");
  
  delay(2000);
}

void loop() {
  server.handleClient();
  
  updateRGBEffect();
  
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 100) {
    updateOLED();
    lastDisplay = millis();
  }
  
  delay(10);
}

// ===== RGB ЭФФЕКТЫ (20 РЕЖИМОВ) =====

void updateRGBEffect() {
  static unsigned long lastUpdate = 0;
  int delayTime = map(101 - speed, 10, 100, 10, 500);
  
  if (millis() - lastUpdate < delayTime) return;
  lastUpdate = millis();
  
  switch(currentMode) {
    case 0: colorStatic(r, g, b); break;
    case 1: colorBreathing(); break;
    case 2: colorRainbow(); break;
    case 3: colorPulse(); break;
    case 4: colorOff(); break;
    case 5: colorStrobe(); break;
    case 6: colorWave(); break;
    case 7: colorFade(); break;
    case 8: colorTwinkle(); break;
    case 9: colorFire(); break;
    case 10: colorWaterfall(); break;
    case 11: colorScanline(); break;
    case 12: colorBounce(); break;
    case 13: colorChase(); break;
    case 14: colorSparkle(); break;
    case 15: colorPulseRainbow(); break;
    case 16: colorCyclone(); break;
    case 17: colorMetallic(); break;
    case 18: colorThunder(); break;
    case 19: colorCandlelight(); break;
  }
}

void colorStatic(int red, int green, int blue) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(red, green, blue));
  }
  strip.show();
}

void colorBreathing() {
  static int dir = 1;
  static int currentBr = 0;
  
  if (currentBr <= 0) dir = 1;
  if (currentBr >= 255) dir = -1;
  
  currentBr += dir * 5;
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(
      (r * currentBr) / 255,
      (g * currentBr) / 255,
      (b * currentBr) / 255
    ));
  }
  strip.show();
}

void colorRainbow() {
  static int hueShift = 0;
  hueShift = (hueShift + 1) % 360;
  
  for (int i = 0; i < strip.numPixels(); i++) {
    int pixelHue = (hueShift + i * 360 / strip.numPixels()) % 360;
    uint32_t color = hsvToRgb(pixelHue, 255, brightness);
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void colorPulse() {
  static int phase = 0;
  phase = (phase + 1) % 360;
  
  int br = 128 + 127 * sin(phase * 3.14159 / 180);
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(
      (r * br) / 255,
      (g * br) / 255,
      (b * br) / 255
    ));
  }
  strip.show();
}

void colorOff() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void colorStrobe() {
  static int strobeCount = 0;
  strobeCount++;
  
  if (strobeCount % 2 == 0) {
    colorStatic(r, g, b);
  } else {
    colorOff();
  }
}

void colorWave() {
  static int wavePos = 0;
  wavePos = (wavePos + 1) % strip.numPixels();
  
  for (int i = 0; i < strip.numPixels(); i++) {
    int dist = abs(i - wavePos);
    int intensity = 255 - (dist * 255 / strip.numPixels());
    strip.setPixelColor(i, strip.Color(
      (r * intensity) / 255,
      (g * intensity) / 255,
      (b * intensity) / 255
    ));
  }
  strip.show();
}

void colorFade() {
  static int fadeCounter = 0;
  fadeCounter++;
  int br = (sin(fadeCounter * 0.05) + 1) * 127;
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(
      (r * br) / 255,
      (g * br) / 255,
      (b * br) / 255
    ));
  }
  strip.show();
}

void colorTwinkle() {
  static unsigned long lastTwinkle = 0;
  if (millis() - lastTwinkle > 100) {
    for (int i = 0; i < strip.numPixels(); i++) {
      if (random(0, 2) == 0) {
        strip.setPixelColor(i, strip.Color(r, g, b));
      } else {
        strip.setPixelColor(i, 0);
      }
    }
    strip.show();
    lastTwinkle = millis();
  }
}

void colorFire() {
  for (int i = 0; i < strip.numPixels(); i++) {
    int heat = random(100, 255);
    int r_fire = 255;
    int g_fire = heat;
    int b_fire = 0;
    strip.setPixelColor(i, strip.Color(r_fire, g_fire, b_fire));
  }
  strip.show();
}

void colorWaterfall() {
  static int waterPos = 0;
  waterPos = (waterPos + 1) % strip.numPixels();
  
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i == waterPos) {
      strip.setPixelColor(i, strip.Color(0, 150, 255));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

void colorScanline() {
  static int scanPos = 0;
  scanPos = (scanPos + 1) % strip.numPixels();
  
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i == scanPos) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

void colorBounce() {
  static int bouncePos = 0;
  static int bounceDir = 1;
  
  bouncePos += bounceDir;
  if (bouncePos <= 0 || bouncePos >= strip.numPixels() - 1) {
    bounceDir = -bounceDir;
  }
  
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i == bouncePos) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

void colorChase() {
  static int chasePos = 0;
  chasePos = (chasePos + 1) % strip.numPixels();
  
  for (int i = 0; i < strip.numPixels(); i++) {
    int dist = abs(i - chasePos);
    if (dist <= 2) {
      int br = 255 - (dist * 85);
      strip.setPixelColor(i, strip.Color(
        (r * br) / 255,
        (g * br) / 255,
        (b * br) / 255
      ));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

void colorSparkle() {
  colorStatic(r / 3, g / 3, b / 3);
  
  int randomLED = random(strip.numPixels());
  strip.setPixelColor(randomLED, strip.Color(r, g, b));
  strip.show();
}

void colorPulseRainbow() {
  static int rainbowPhase = 0;
  rainbowPhase = (rainbowPhase + 1) % 360;
  
  int br = 128 + 127 * sin(rainbowPhase * 3.14159 / 180);
  
  for (int i = 0; i < strip.numPixels(); i++) {
    int pixelHue = (rainbowPhase + i * 360 / strip.numPixels()) % 360;
    uint32_t color = hsvToRgb(pixelHue, 255, br);
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void colorCyclone() {
  static int cyclonePos = 0;
  cyclonePos = (cyclonePos + 1) % strip.numPixels();
  
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i == cyclonePos || i == (cyclonePos + strip.numPixels() / 2) % strip.numPixels()) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

void colorMetallic() {
  static int metallicPhase = 0;
  metallicPhase = (metallicPhase + 1) % 360;
  
  for (int i = 0; i < strip.numPixels(); i++) {
    int br = 100 + 155 * sin((metallicPhase + i * 30) * 3.14159 / 180);
    strip.setPixelColor(i, strip.Color(
      (200 * br) / 255,
      (200 * br) / 255,
      (200 * br) / 255
    ));
  }
  strip.show();
}

void colorThunder() {
  static unsigned long lastThunder = 0;
  static int thunderIntensity = 0;
  
  if (millis() - lastThunder > 100) {
    if (random(0, 10) == 0) {
      thunderIntensity = 255;
    } else {
      thunderIntensity = std::max(0, thunderIntensity - 30);
    }
    lastThunder = millis();
  }
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(
      thunderIntensity,
      thunderIntensity,
      (thunderIntensity * 2) / 3
    ));
  }
  strip.show();
}

void colorCandlelight() {
  static int candleFlicker = 200;
  candleFlicker = std::max(50, (int)(200 + random(-50, 51)));
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(
      255,
      (200 * candleFlicker) / 255,
      0
    ));
  }
  strip.show();
}

uint32_t hsvToRgb(int h, int s, int v) {
  float hf = h / 60.0;
  int i = (int)hf;
  float f = hf - i;
  
  int p = v * (100 - s) / 100;
  int q = v * (100 - s * f) / 100;
  int t = v * (100 - s * (1 - f)) / 100;
  
  int red, green, blue;
  
  switch(i) {
    case 0: red = v; green = t; blue = p; break;
    case 1: red = q; green = v; blue = p; break;
    case 2: red = p; green = v; blue = t; break;
    case 3: red = p; green = q; blue = v; break;
    case 4: red = t; green = p; blue = v; break;
    default: red = v; green = p; blue = q;
  }
  
  return strip.Color(red, green, blue);
}

// ===== OLED =====

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  
  display.println("RGB CONTROLLER");
  display.println("");
  
  display.print("Mode: ");
  display.println(modeNames[currentMode]);
  
  display.print("Theme: ");
  display.println(themes[currentTheme].name);
  
  display.print("RGB: ");
  display.print(r); display.print(",");
  display.print(g); display.print(",");
  display.println(b);
  
  display.print("Brightness: ");
  display.println(brightness);
  
  display.display();
}

// ===== ВЕБ-СЕРВЕР =====

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    String html = getHTMLPage();
    server.send(200, "text/html", html);
  });
  
  server.on("/api/mode", HTTP_POST, []() {
    if (server.hasArg("value")) {
      currentMode = server.arg("value").toInt() % 20;
      saveSettings();
      Serial.print("Mode: ");
      Serial.println(modeNames[currentMode]);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/api/theme", HTTP_POST, []() {
    if (server.hasArg("value")) {
      currentTheme = server.arg("value").toInt() % 10;
      r = themes[currentTheme].r;
      g = themes[currentTheme].g;
      b = themes[currentTheme].b;
      saveSettings();
      Serial.print("Theme: ");
      Serial.println(themes[currentTheme].name);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/api/brightness", HTTP_POST, []() {
    if (server.hasArg("value")) {
      brightness = constrain(server.arg("value").toInt(), 0, 255);
      saveSettings();
    }
    server.send(200, "text/plain", String(brightness));
  });
  
  server.on("/api/speed", HTTP_POST, []() {
    if (server.hasArg("value")) {
      speed = constrain(server.arg("value").toInt(), 10, 100);
      saveSettings();
    }
    server.send(200, "text/plain", String(speed));
  });
  
  server.on("/api/color", HTTP_POST, []() {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
      r = constrain(server.arg("r").toInt(), 0, 255);
      g = constrain(server.arg("g").toInt(), 0, 255);
      b = constrain(server.arg("b").toInt(), 0, 255);
      saveSettings();
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}

String getHTMLPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RGB Controller Pro</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(135deg, #1e1e2e 0%, #2d2d3d 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container {
      background: rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(10px);
      border-radius: 20px;
      padding: 30px;
      max-width: 600px;
      margin: 0 auto;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    h1 { text-align: center; color: #00d4ff; margin-bottom: 30px; font-size: 28px; }
    .section { margin-bottom: 30px; }
    .section-title { color: #00d4ff; font-weight: bold; margin-bottom: 15px; font-size: 14px; text-transform: uppercase; }
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 8px;
      margin-bottom: 20px;
    }
    .theme-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 8px;
      margin-bottom: 20px;
    }
    .btn {
      padding: 10px;
      border: 2px solid #00d4ff;
      background: transparent;
      color: #00d4ff;
      border-radius: 6px;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      transition: all 0.2s;
    }
    .btn:hover { background: #00d4ff; color: #1e1e2e; }
    .btn.active { background: #00d4ff; color: #1e1e2e; box-shadow: 0 0 10px rgba(0, 212, 255, 0.5); }
    .control-group { margin-bottom: 20px; }
    label { display: block; color: #ffffff; margin-bottom: 8px; font-weight: 600; font-size: 13px; }
    .slider-container { display: flex; gap: 10px; align-items: center; }
    input[type="range"] { flex: 1; }
    .value-display { min-width: 40px; text-align: center; color: #00d4ff; font-weight: bold; }
    .color-picker { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; margin-top: 10px; }
    input[type="color"] { width: 100%; height: 50px; border: none; border-radius: 6px; cursor: pointer; }
    input[type="number"] { padding: 6px; background: rgba(255, 255, 255, 0.1); border: 1px solid rgba(0, 212, 255, 0.3); color: #ffffff; border-radius: 4px; width: 100%; }
    .color-preview { width: 100%; height: 30px; border-radius: 6px; border: 2px solid #00d4ff; margin-top: 10px; }
    .status { background: rgba(0, 212, 255, 0.1); border-left: 4px solid #00d4ff; padding: 15px; border-radius: 5px; margin-top: 20px; font-size: 12px; color: #ffffff; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎨 RGB Controller PRO</h1>
    
    <div class="section">
      <div class="section-title">📋 Режимы (20)</div>
      <div class="mode-grid" id="modeGrid"></div>
    </div>
    
    <div class="section">
      <div class="section-title">🎨 Темы (10)</div>
      <div class="theme-grid" id="themeGrid"></div>
    </div>
    
    <div class="section">
      <div class="section-title">⚙️ Параметры</div>
      <div class="control-group">
        <label>Яркость: <span id="brightnessValue">255</span></label>
        <div class="slider-container">
          <input type="range" id="brightness" min="0" max="255" value="255" onchange="setBrightness(this.value)">
        </div>
      </div>
      
      <div class="control-group">
        <label>Скорость: <span id="speedValue">50</span></label>
        <div class="slider-container">
          <input type="range" id="speed" min="10" max="100" value="50" onchange="setSpeed(this.value)">
        </div>
      </div>
      
      <div class="control-group">
        <label>Цвет (RGB)</label>
        <div class="color-picker">
          <input type="color" id="colorPicker" onchange="updateColorFromPicker()">
          <input type="number" id="redValue" min="0" max="255" value="255" onchange="setColor()" placeholder="R">
          <input type="number" id="greenValue" min="0" max="255" value="0" onchange="setColor()" placeholder="G">
          <input type="number" id="blueValue" min="0" max="255" value="0" onchange="setColor()" placeholder="B">
        </div>
        <div class="color-preview" id="colorPreview"></div>
      </div>
    </div>
    
    <div class="status">
      <p><strong>Текущий режим:</strong> <span id="modeStatus">Static</span></p>
      <p><strong>Текущая тема:</strong> <span id="themeStatus">Красный</span></p>
      <p><strong>Статус:</strong> ✓ Подключено</p>
    </div>
  </div>

  <script>
    const modes = ["Static", "Breathing", "Rainbow", "Pulse", "Off", "Strobe", "Wave", "Fade", "Twinkle", "Fire", "Waterfall", "Scanline", "Bounce", "Chase", "Sparkle", "PulseRainbow", "Cyclone", "Metallic", "Thunder", "Candle"];
    const themes = ["Красный", "Синий", "Зелёный", "Фиолетовый", "Оранжевый", "Голубой", "Розовый", "Жёлтый", "Лайм", "Белый"];
    
    let currentMode = 0;
    let currentTheme = 0;
    
    document.addEventListener("DOMContentLoaded", () => {
      const modeGrid = document.getElementById("modeGrid");
      modes.forEach((mode, idx) => {
        const btn = document.createElement("button");
        btn.className = "btn" + (idx === 0 ? " active" : "");
        btn.textContent = mode;
        btn.onclick = () => setMode(idx);
        modeGrid.appendChild(btn);
      });
      
      const themeGrid = document.getElementById("themeGrid");
      themes.forEach((theme, idx) => {
        const btn = document.createElement("button");
        btn.className = "btn" + (idx === 0 ? " active" : "");
        btn.textContent = theme;
        btn.onclick = () => setTheme(idx);
        themeGrid.appendChild(btn);
      });
      
      updatePreview(255, 0, 0);
    });
    
    async function setMode(mode) {
      currentMode = mode;
      await fetch("/api/mode", {method: "POST", body: new URLSearchParams({value: mode})});
      document.querySelectorAll(".mode-grid .btn").forEach((b, i) => b.classList.toggle("active", i === mode));
      document.getElementById("modeStatus").textContent = modes[mode];
    }
    
    async function setTheme(theme) {
      currentTheme = theme;
      await fetch("/api/theme", {method: "POST", body: new URLSearchParams({value: theme})});
      document.querySelectorAll(".theme-grid .btn").forEach((b, i) => b.classList.toggle("active", i === theme));
      document.getElementById("themeStatus").textContent = themes[theme];
    }
    
    async function setBrightness(value) {
      document.getElementById("brightnessValue").textContent = value;
      await fetch("/api/brightness", {method: "POST", body: new URLSearchParams({value: value})});
    }
    
    async function setSpeed(value) {
      document.getElementById("speedValue").textContent = value;
      await fetch("/api/speed", {method: "POST", body: new URLSearchParams({value: value})});
    }
    
    async function setColor() {
      const r = document.getElementById("redValue").value || 0;
      const g = document.getElementById("greenValue").value || 0;
      const b = document.getElementById("blueValue").value || 0;
      updatePreview(r, g, b);
      await fetch("/api/color", {method: "POST", body: new URLSearchParams({r, g, b})});
    }
    
    function updateColorFromPicker() {
      const hex = document.getElementById("colorPicker").value;
      const r = parseInt(hex.slice(1, 3), 16);
      const g = parseInt(hex.slice(3, 5), 16);
      const b = parseInt(hex.slice(5, 7), 16);
      document.getElementById("redValue").value = r;
      document.getElementById("greenValue").value = g;
      document.getElementById("blueValue").value = b;
      setColor();
    }
    
    function updatePreview(r, g, b) {
      const color = `rgb(${r}, ${g}, ${b})`;
      document.getElementById("colorPreview").style.background = color;
      const hex = "#" + [r, g, b].map(x => {
        const h = x.toString(16);
        return h.length === 1 ? "0" + h : h;
      }).join("").toUpperCase();
      document.getElementById("colorPicker").value = hex;
    }
    
    setMode(0);
    setTheme(0);
  </script>
</body>
</html>
  )rawliteral";
  
  return html;
}
