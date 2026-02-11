#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// --- Configuration ---
const char* ssid = "YOUR_WIFI_SSID";         // REPLACE ME
const char* password = "YOUR_WIFI_PASSWORD"; // REPLACE ME

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);

// --- Eye State & Emotions ---
enum Emotion { NORMAL, HAPPY, ANGRY, SLEEPY, EXCITED, GLITCH, SCAN, SHAKING };
Emotion currentEmotion = NORMAL;
unsigned long emotionStartTime = 0;
const int REF_EYE_HEIGHT = 40;
const int REF_EYE_WIDTH = 40;
const int REF_SPACE_BETWEEN_EYE = 10;
const int REF_CORNER_RADIUS = 10;

struct EyeState {
  int x, y, width, height;
};
EyeState left_eye, right_eye;
int corner_radius = REF_CORNER_RADIUS;

// --- Mini Game State ---
bool inGameMode = false;
int playerY = 40;
int playerVelocity = 0;
int obstacleX = 128;
int score = 0;
bool gameOver = false;

// --- Time Sync ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800; // IST (UTC+5:30)
const int   daylightOffset_sec = 0;

// --- Web UI Content (Cyberpunk) ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NEO-PET | CORE</title>
    <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&family=Rajdhani:wght@500;700&display=swap" rel="stylesheet">
    <style>
        :root { --n-blue: #00f3ff; --n-purple: #bc13fe; --bg: #050505; }
        body { background: var(--bg); color: #fff; font-family: 'Rajdhani', sans-serif; display: flex; flex-direction: column; align-items: center; padding: 20px; text-align: center; }
        .header h1 { font-family: 'Orbitron', sans-serif; background: linear-gradient(to right, var(--n-blue), var(--n-purple)); -webkit-background-clip: text; -webkit-text-fill-color: transparent; text-shadow: 0 0 10px rgba(0,243,255,0.5); font-size: 2.5rem; margin: 10px 0; }
        .status { background: rgba(255,255,255,0.05); padding: 10px 20px; border-radius: 50px; border: 1px solid rgba(255,255,255,0.1); margin-bottom: 20px; color: var(--n-blue); font-size: 0.9rem; letter-spacing: 1px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; width: 100%; max-width: 400px; }
        .btn { background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.1); padding: 20px; border-radius: 15px; cursor: pointer; transition: 0.3s; font-weight: bold; text-transform: uppercase; }
        .btn:active { transform: scale(0.95); border-color: var(--n-blue); background: rgba(0,243,255,0.1); }
        .game-title { margin-top: 30px; font-family: 'Orbitron'; font-size: 0.7rem; color: var(--n-purple); opacity: 0.7; }
        .joy-btn { width: 100%; max-width: 400px; height: 100px; background: linear-gradient(135deg, rgba(188,19,254,0.1), rgba(0,243,255,0.1)); border: 2px dashed var(--n-purple); border-radius: 20px; display: flex; align-items: center; justify-content: center; font-size: 1.5rem; font-family: 'Orbitron'; color: var(--n-purple); margin-top: 10px; cursor: pointer; }
        .joy-btn:active { background: var(--n-purple); color: #fff; }
    </style>
</head>
<body>
    <div class="header"><h1>NEO•PET</h1></div>
    <div class="status" id="clock">SYSTEM INITIALIZING...</div>
    <div class="grid">
        <div class="btn" onclick="act('happy')">Happy</div>
        <div class="btn" onclick="act('angry')">Angry</div>
        <div class="btn" onclick="act('excited')">Excited</div>
        <div class="btn" onclick="act('sleepy')">Sleepy</div>
        <div class="btn" onclick="act('glitch')">Glitch</div>
        <div class="btn" onclick="act('scan')">Scan</div>
    </div>
    <div class="game-title">EXPERIMENTAL NEORUNNER</div>
    <div class="joy-btn" onclick="act('jump')">JUMP / START</div>
    <script>
        function act(t) { fetch('/act?type=' + t); }
        setInterval(() => {
            const now = new Date();
            document.getElementById('clock').innerText = now.toLocaleTimeString();
        }, 1000);
    </script>
</body>
</html>
)rawliteral";

// --- Helper Functions ---

void reset_eyes_state() {
  left_eye.width = right_eye.width = REF_EYE_WIDTH;
  left_eye.height = right_eye.height = REF_EYE_HEIGHT;
  left_eye.x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  left_eye.y = SCREEN_HEIGHT / 2;
  right_eye.x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  right_eye.y = SCREEN_HEIGHT / 2;
  corner_radius = REF_CORNER_RADIUS;
}

void draw_eyes() {
  display.clearDisplay();
  display.fillRoundRect(left_eye.x - left_eye.width / 2, left_eye.y - left_eye.height / 2, left_eye.width, left_eye.height, corner_radius, SSD1306_WHITE);
  display.fillRoundRect(right_eye.x - right_eye.width / 2, right_eye.y - right_eye.height / 2, right_eye.width, right_eye.height, corner_radius, SSD1306_WHITE);
  display.display();
}

void show_time() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  display.setRotation(0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SYST-TIME:");
  display.setTextSize(2);
  display.setCursor(10, 25);
  char timeStr[10];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  display.println(timeStr);
  display.display();
  delay(3000);
}

// --- Specific Animations ---

void animate_happy() {
  reset_eyes_state();
  for (int i = 0; i < 5; i++) {
    display.clearDisplay();
    int curve = i * 4;
    // Draw semi-circles for happy eyes
    display.fillRoundRect(left_eye.x - 20, left_eye.y - 10, 40, 20, 10, SSD1306_WHITE);
    display.fillRoundRect(right_eye.x - 20, right_eye.y - 10, 40, 20, 10, SSD1306_WHITE);
    display.fillRect(left_eye.x - 20, left_eye.y, 40, 10, SSD1306_BLACK);
    display.fillRect(right_eye.x - 20, right_eye.y, 40, 10, SSD1306_BLACK);
    display.display();
    delay(50);
  }
}

void animate_excited() {
  for (int i = 0; i < 10; i++) {
    int s = 30 + (i % 2 == 0 ? 10 : -5);
    display.clearDisplay();
    display.fillCircle(left_eye.x, left_eye.y, s / 2, SSD1306_WHITE);
    display.fillCircle(right_eye.x, right_eye.y, s / 2, SSD1306_WHITE);
    display.display();
    delay(100);
  }
}

void animate_sleepy() {
  reset_eyes_state();
  for (int h = REF_EYE_HEIGHT; h > 5; h -= 2) {
    left_eye.height = right_eye.height = h;
    draw_eyes();
    delay(50);
  }
  delay(2000);
}

void animate_blink() {
  int original_h = left_eye.height;
  left_eye.height = right_eye.height = 2;
  draw_eyes();
  delay(80);
  left_eye.height = right_eye.height = original_h;
  draw_eyes();
}

// --- Game Logic ---

void reset_game() {
  score = 0;
  obstacleX = 120;
  playerY = 40;
  gameOver = false;
  inGameMode = true;
}

void run_game_tick() {
  display.clearDisplay();
  
  // Gravity
  playerVelocity += 1;
  playerY += playerVelocity;
  if (playerY > 40) { playerY = 40; playerVelocity = 0; }
  
  // Player (Eye)
  display.fillRoundRect(20, playerY, 15, 15, 4, SSD1306_WHITE);
  
  // Obstacle
  obstacleX -= 4;
  if (obstacleX < -10) { obstacleX = 128; score++; }
  display.fillRect(obstacleX, 40, 10, 15, SSD1306_WHITE);
  
  // Score
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("XP: "); display.print(score);
  
  // Collision
  if (obstacleX < 35 && obstacleX > 10 && playerY > 25) {
    gameOver = true;
  }
  
  if (gameOver) {
    display.setCursor(30, 25);
    display.setTextSize(2);
    display.println("HALT!");
    display.display();
    delay(2000);
    inGameMode = false;
  } else {
    display.display();
  }
}

// --- Server Handlers ---

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleAct() {
  String type = server.arg("type");
  if (type == "happy") currentEmotion = HAPPY;
  else if (type == "angry") currentEmotion = ANGRY;
  else if (type == "excited") currentEmotion = EXCITED;
  else if (type == "sleepy") currentEmotion = SLEEPY;
  else if (type == "glitch") currentEmotion = GLITCH;
  else if (type == "scan") currentEmotion = SCAN;
  else if (type == "jump") {
    if (!inGameMode) reset_game();
    else playerVelocity = -8;
  }
  server.send(200, "text/plain", "OK");
  emotionStartTime = millis();
}

// --- Setup & Loop ---

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("NEO-PET BOOTING...");
  display.display();

  // WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nCONNECTED!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    display.println("WIFI LINK ACTIVE");
    display.println(WiFi.localIP().toString());
  } else {
    display.println("OFFLINE MODE");
  }
  display.display();
  delay(2000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  server.on("/", handleRoot);
  server.on("/act", handleAct);
  server.begin();
  
  reset_eyes_state();
  animate_happy();
}

void loop() {
  server.handleClient();

  if (inGameMode) {
    run_game_tick();
    return;
  }

  // Handle Emotions
  if (currentEmotion != NORMAL) {
    switch (currentEmotion) {
      case HAPPY: animate_happy(); break;
      case EXCITED: animate_excited(); break;
      case SLEEPY: animate_sleepy(); break;
      case ANGRY: 
        // Simple angry frame
        display.clearDisplay();
        display.fillRoundRect(left_eye.x - 20, left_eye.y - 12, 40, 30, 10, SSD1306_WHITE);
        display.fillRoundRect(right_eye.x - 20, right_eye.y - 12, 40, 30, 10, SSD1306_WHITE);
        display.fillTriangle(left_eye.x-22, left_eye.y-14, left_eye.x+22, left_eye.y-14, left_eye.x+22, left_eye.y+2, SSD1306_BLACK);
        display.fillTriangle(right_eye.x+22, right_eye.y-14, right_eye.x-22, right_eye.y-14, right_eye.x-22, right_eye.y+2, SSD1306_BLACK);
        display.display();
        delay(2000);
        break;
      case GLITCH:
        for(int i=0; i<10; i++) {
          display.clearDisplay();
          display.fillRect(random(0, 128), random(0, 64), 20, 5, SSD1306_WHITE);
          display.display();
          delay(50);
        }
        break;
      case SCAN:
        for(int x=0; x<128; x+=8) {
          display.clearDisplay();
          display.drawFastVLine(x, 0, 64, SSD1306_WHITE);
          display.display();
          delay(20);
        }
        break;
      case SHAKING:
        for (int i = 0; i < 15; i++) {
          display.clearDisplay();
          int ox = random(-4, 5);
          int oy = random(-4, 5);
          display.fillRoundRect(left_eye.x - 20 + ox, left_eye.y - 20 + oy, 40, 40, 10, SSD1306_WHITE);
          display.fillRoundRect(right_eye.x - 20 + ox, right_eye.y - 20 + oy, 40, 40, 10, SSD1306_WHITE);
          display.display();
          delay(40);
        }
        break;
    }
    currentEmotion = NORMAL;
    reset_eyes_state();
  } else {
    // Idle behavior
    draw_eyes();
    if (random(0, 100) > 97) animate_blink();
    if (random(0, 500) > 498) {
        // Random look around (Saccade)
        int dx = random(-10, 11);
        int dy = random(-5, 6);
        left_eye.x += dx; right_eye.x += dx;
        left_eye.y += dy; right_eye.y += dy;
        draw_eyes();
        delay(random(500, 1500));
        reset_eyes_state();
    }
    if (random(0, 5000) > 4998) show_time();
    delay(50);
  }
}

