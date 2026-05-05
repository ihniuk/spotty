# 🎵 Spotty-CYD 
### *The High-Polish Spotify Controller for ESP32 CYD*

> [!IMPORTANT]
> **Status: Work In Progress (WIP)**
> This project is currently under active development. Expect frequent updates and potential breaking changes as we polish the UI and optimize API interactions.

Spotty-CYD is a premium, feature-rich Spotify controller designed specifically for the **Cheap Yellow Display (CYD)** (ESP32-2432S028R). It focuses on providing a buttery-smooth, flicker-free experience while respecting Spotify's strict API rate limits.

![Project Preview](https://via.placeholder.com/640x480.png?text=Spotty-CYD+UI+Preview) <!-- Replace with real image later -->

## ✨ Features

- **🎨 Premium UI:** Custom-built rendering engine using `TFT_eSprite` for zero-flicker animations.
- **🔄 Fully Responsive:** Dynamic layout engine that re-anchors elements for all 4 orientations (Portrait/Landscape).
- **⚙️ Paginated Settings:** A modern 4-page menu for Rotation, Brightness, Time Zone, and System Info.
- **🌍 Global Time Sync:** On-screen GMT Offset adjustment for worldwide local time support.
- **🛡️ Safety Reset:** Interactive factory reset confirmation screen to prevent accidental data loss.
- **📶 System Info:** Live display of local IP address, software version, and build date.
- **🎨 Modern Aesthetics:** Refined slider-based settings icon and visual touch feedback (grey-flashing buttons).
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
3. Add `https://google.com/` to your **Redirect URIs**.
4. Note your **Client ID** and **Client Secret**.

### 2. Installation
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

