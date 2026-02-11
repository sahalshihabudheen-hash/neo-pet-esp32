#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "HOME";
const char* password = "pass@123";

Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);

int emotion = 0; // 0:Idle, 1:Happy, 2:Angry, 3:Sleepy, 4:Excited, 5:Glitch, 6:Scan, 7:Game
int eye_x = 0, eye_y = 0;
int playerY = 40, velocity = 0, obsX = 128, score = 0;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body{background:#050505;color:#00f3ff;font-family:sans-serif;text-align:center;padding:10px;}
.btn{background:rgba(0,243,255,0.1);border:1px solid #00f3ff;color:#00f3ff;padding:15px;margin:5px;width:40%;border-radius:10px;font-weight:bold;box-shadow:0 0 10px #00f3ff;transition:0.2s;}
.btn:active{background:#00f3ff;color:#000;}
.game{background:linear-gradient(to right, #bc13fe, #00f3ff);color:#000;width:90%;height:80px;font-size:1.5rem;margin-top:20px;}
</style></head><body><h1>NEO•PET CORE</h1>
<button class="btn" onclick="fetch('/act?t=1')">HAPPY</button>
<button class="btn" onclick="fetch('/act?t=2')">ANGRY</button>
<button class="btn" onclick="fetch('/act?t=4')">EXCITED</button>
<button class="btn" onclick="fetch('/act?t=3')">SLEEPY</button>
<button class="btn" onclick="fetch('/act?t=5')">GLITCH</button>
<button class="btn" onclick="fetch('/act?t=6')">SCAN</button><br>
<button class="btn game" onclick="fetch('/act?t=7')">JUMP / START</button>
<script>function act(t){fetch('/act?t='+t);}</script></body></html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }
void handleAct() {
  int t = server.arg("t").toInt();
  if (t == 7) { 
    if (emotion != 7) { emotion = 7; score = 0; obsX = 120; } 
    else { velocity = -8; } 
  } else { emotion = t; }
  server.send(200, "text/plain", "OK");
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay(); display.setTextColor(1);
  display.println("NEO-PET BOOT..."); display.display();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  display.println("CONNECTED!"); display.println(WiFi.localIP()); display.display();
  server.on("/", handleRoot);
  server.on("/act", handleAct);
  server.begin();
  delay(2000);
}

void draw_eyes(int x, int y, int h) {
  display.fillRoundRect(25+x, 20+y, 35, h, 8, 1);
  display.fillRoundRect(68+x, 20+y, 35, h, 8, 1);
}

void loop() {
  server.handleClient();
  display.clearDisplay();

  if (emotion == 7) { // MINI GAME
    velocity += 1; playerY += velocity;
    if (playerY > 40) { playerY = 40; velocity = 0; }
    obsX -= 5; if (obsX < -10) { obsX = 128; score++; }
    display.fillRoundRect(20, playerY, 15, 15, 4, 1);
    display.fillRect(obsX, 40, 10, 15, 1);
    display.setCursor(0,0); display.print("XP: "); display.print(score);
    if (obsX < 35 && obsX > 10 && playerY > 25) { 
        display.setCursor(35, 25); display.setTextSize(2); display.print("BOOM!"); 
        display.display(); delay(2000); emotion = 0; 
    }
  } else {
    if (emotion == 1) { // Happy
      draw_eyes(eye_x, eye_y, 35); display.fillRect(0, 35, 128, 30, 0);
    } else if (emotion == 2) { // Angry
      draw_eyes(eye_x, eye_y, 35); display.fillTriangle(0,0,128,0,64,30,0);
    } else if (emotion == 4) { // Excited
      int s = (millis()%400<200)?35:20;
      display.fillCircle(42+eye_x, 32, s/2, 1); display.fillCircle(85+eye_x, 32, s/2, 1);
    } else if (emotion == 3) { // Sleepy
      display.fillRect(25+eye_x, 35, 35, 5, 1); display.fillRect(68+eye_x, 35, 35, 5, 1);
    } else if (emotion == 5) { // Glitch
      for(int i=0; i<5; i++) display.drawFastHLine(0, random(0,64), 128, 1);
      draw_eyes(random(-10,10), random(-5,5), random(10,40));
    } else if (emotion == 6) { // Scan
      int sweep = (millis()/5) % 128;
      display.drawFastVLine(sweep, 0, 64, 1); draw_eyes(0, 5, 5);
    } else { // Idle
      if (random(0, 100) > 98) { eye_x = random(-10, 11); eye_y = random(-5, 6); }
      if (random(0, 100) > 96) { display.fillRect(25+eye_x, 35, 35, 4, 1); display.fillRect(68+eye_x, 35, 35, 4, 1); }
      else { draw_eyes(eye_x, eye_y, 35); }
    }
    if (emotion != 0) { 
      static unsigned long start = 0; if(start==0) start=millis();
      if(millis()-start > 4000) { emotion = 0; start = 0; }
    }
  }
  display.display();
  delay(30);
}
