# 🌌 NEO-PET | Cyberpunk AI Desktop Companion

A futuristic AI desktop pet powered by **ESP32** with a 0.96" OLED display. Featuring expressive animations, a mobile control portal, and built-in mini-games.

![Project Status](https://img.shields.io/badge/Status-Active-brightgreen)
![Design](https://img.shields.io/badge/Design-Cyberpunk-blueviolet)

## ✨ Features
- **Expressive Eyes**: Smooth animations for Happy, Angry, Sleepy, Excited, and more.
- **Web Portal**: Mobile-friendly neon cyberpunk UI to control emotions.
- **Mini-Games**: Play "NeoRunner" on the OLED using your phone as a controller.
- **NTP Time Sync**: Real-time clock display synced via WiFi.
- **Autonomous Behavior**: Random blinks, saccades, and glitches for a lifelike feel.

## 🚀 Getting Started

### 1. Prerequisites
- ESP32 Development Board
- 0.96" I2C OLED Display (SSD1306)
- Arduino IDE with ESP32 board support

### 2. Libraries Required
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `WiFi` & `WebServer` (Built-in)

### 3. Installation
1. Clone this repository:
   ```bash
   git clone https://github.com/YOUR_USERNAME/neo-pet-esp32.git
   ```
2. Open `animated_eye_jarvis.ino` in Arduino IDE.
3. Update your WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   ```
4. Upload to your ESP32.
5. Open the Serial Monitor to find the local IP address.
6. Access the IP in your mobile browser to start the UI!

## 🎮 Controls
- **Emotions**: Select from the grid to change the pet's mood instantly.
- **NeoRunner**: Tap the "JUMP" button to start the game and jump over obstacles.

---
*Created with 💜 for the Cyberpunk aesthetic.*
