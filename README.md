# 🎵 Spotty-CYD 
### *The High-Polish Spotify Controller for ESP32 CYD*

> [!IMPORTANT]
> **Status: Work In Progress (WIP)**
> This project is currently under active development. Expect frequent updates and potential breaking changes as we polish the UI and optimize API interactions.

Spotty-CYD is a premium, feature-rich Spotify controller designed specifically for the **Cheap Yellow Display (CYD)** (ESP32-2432S028R). It focuses on providing a buttery-smooth, flicker-free experience while respecting Spotify's strict API rate limits.

![Project Preview](https://via.placeholder.com/640x480.png?text=Spotty-CYD+UI+Preview) <!-- Replace with real image later -->

## ✨ Features

- **🎨 Premium UI:** Custom-built rendering engine using `TFT_eSprite` for zero-flicker animations.
- **🧈 Fluid Scrolling:** Infinite, liquid-smooth text scrolling for long track names and podcast titles.
- **🛡️ Smart Rate-Limit Shield:** Implements exponential backoff (60s -> 120s -> 240s) and on-screen countdowns when Spotify throttles requests.
- **🔮 Predictive Progress:** Animates the progress bar in real-time between API updates, reducing network calls by up to 70%.
- **📉 Variable Polling:** Automatically adjusts check-in frequency (10s when playing, 3s near track end, 30s when paused).
- **⚙️ On-Device Settings:** Adjust screen brightness and rotation directly from the touch interface.
- **🖼️ Album Art:** High-quality JPEG decoding and display.

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
3. Update `ConfigManager.h` or use the on-screen setup (if implemented) to provide your credentials.
4. Build and Upload to your CYD.

## 📦 Dependencies

- `TFT_eSPI` (Display)
- `SpotifyArduino` (API)
- `JPEGDEC` (Artwork)
- `ArduinoJson` (Data Parsing)
- `CYD28_TouchscreenR` (Touch)

---

## 🏗️ Roadmap
- [ ] Multi-device playback switching.
- [ ] Volume control slider.
- [ ] Wi-Fi configuration portal (Captive Portal).
- [ ] Improved font rendering for international characters.

---
*Created with ❤️ for the ESP32 community.*
