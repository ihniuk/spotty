# 🎵 Spotty-CYD 
### *The High-Polish Spotify Controller for ESP32 CYD*

> [!IMPORTANT]
> **Status: Work In Progress (WIP)**
> This project is currently under active development. Expect frequent updates and potential breaking changes as we polish the UI and optimize API interactions.

Spotty-CYD is a premium, feature-rich Spotify controller designed specifically for the **Cheap Yellow Display (CYD)** (ESP32-2432S028R). It focuses on providing a buttery-smooth, flicker-free experience while respecting Spotify's strict API rate limits.

![Project Preview](https://via.placeholder.com/640x480.png?text=Spotty-CYD+UI+Preview) <!-- Replace with real image later -->

## ✨ Features

- **🎨 Premium UI:** Custom-built rendering engine using `TFT_eSprite` for zero-flicker animations.
- **⚡ Zero-Touch Web Provisioning:** Flash the firmware and securely inject WiFi + Spotify credentials directly from your browser via the Web Serial API.
- **⏱️ Live Track Countdown:** Real-time elapsed and remaining time labels that sync perfectly with the progress bar.
- **📶 Dynamic WiFi Meter:** Working signal strength (RSSI) icon that provides live feedback on network health.
- **🎮 Modern Playback Controls:** High-polish, circular UI buttons designed to mimic the premium Spotify experience.
- **🔄 Fully Responsive:** Dynamic layout engine that re-anchors elements for all 4 orientations (Portrait/Landscape).
- **⚙️ Paginated Settings:** A modern 4-page menu for Rotation, Brightness, Time Zone, and System Info.
- **🌍 Global Time Sync:** On-screen GMT Offset adjustment for worldwide local time support.
- **🛡️ Safety Reset:** Interactive factory reset confirmation screen to prevent accidental data loss.
- **🧈 Fluid Scrolling:** Infinite, liquid-smooth text scrolling for long track names.
- **🔮 Predictive Progress:** Real-time progress bar animation reduces network calls by up to 70%.

## 🛠️ Hardware Requirements

- **Device:** ESP32-2432S028R (Cheap Yellow Display).
- **USB:** Micro-USB for power and flashing.
- **Touch:** Integrated resistive touch support.

## 🚀 Getting Started

### 1. Spotify Developer Setup
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard).
2. Create a new App.
3. Add `https://google.com/` and capture the URL manually if you prefer the old fallback method.
4. Note your **Client ID** and **Client Secret**.

### 2. Installation (Web Installer - Recommended)
The easiest way to install Spotty-CYD is via the Web Installer (Requires Google Chrome or Microsoft Edge):
1. Navigate to the Web Configurator *(External link coming soon)*.
2. Enter your WiFi and Spotify Developer details directly into the web page.
3. Plug in your CYD via USB.
4. Click **Install & Configure**. The system will automatically flash the firmware and inject your credentials into the chip's raw memory.
5. On the first boot, it will connect immediately to WiFi without requiring a captive portal.

### 3. Manual Installation (PlatformIO)
1. Clone this repository.
2. Open in **VS Code** with the **PlatformIO** extension.
3. Build and Upload to your CYD.
4. On first boot, connect to the `Spotify-CYD-Setup` Wi-Fi to enter your credentials.

## 📦 Dependencies

- `TFT_eSPI` (Display)
- `SpotifyArduino` (API)
- `JPEGDEC` (Artwork)
- `ArduinoJson` (Data Parsing)
- `CYD28_TouchscreenR` (Touch)
- `WiFiManager` (Provisioning)

---

## 🏗️ Roadmap
- [ ] Multi-device playback switching.
- [ ] Volume control slider.
- [x] Wi-Fi configuration portal (Captive Portal).
- [ ] Improved font rendering for international characters.

---
*Created with ❤️ for the ESP32 community.*

