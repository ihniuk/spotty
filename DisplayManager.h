#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <FS.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecure.h>
#include <qrcode.h>
#include <time.h>

class DisplayManager {
private:
  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite spr = TFT_eSprite(&tft);
  JPEGDEC jpeg;
  String _currentTitle = "No Music";
  String _currentArtist = "Playing";
  bool _isPlaying = false;
  int _scrollOffset = 0;
  int _titleWidth = 0;
  unsigned long _lastScrollTime = 0;
  float _lastProgress = 0;

  static int jpegDraw(JPEGDRAW *pDraw) {
    TFT_eSPI *tft = (TFT_eSPI *)pDraw->pUser;
    tft->pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight,
                   pDraw->pPixels);
    return 1;
  }

public:
  void begin(int rotation) {
    tft.init();
    tft.setRotation(rotation);
    tft.invertDisplay(true);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Backlight setup
    ledcSetup(0, 5000, 8);
    ledcAttachPin(21, 0);

    // Create a sprite for the bottom UI area (Track Info + Progress)
    // 320x60 is enough for the bottom section
    spr.createSprite(320, 65);
  }

  void setBrightness(int level) {
    // level 1-5
    int val = map(level, 1, 5, 20, 255);
    ledcWrite(0, val);
  }

  void showLoading(const char *msg) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 20);
    tft.setTextSize(2);
    tft.println(msg);
  }

  // New: Local QR Code Generator (No Internet Needed)
  void showQRCode(const char *text) {
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, text);

    int scale = 4;
    int offset_x = (320 - (qrcode.size * scale)) / 2;
    int offset_y = 60;

    tft.fillRect(offset_x - 10, offset_y - 10, (qrcode.size * scale) + 20,
                 (qrcode.size * scale) + 20, TFT_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
        if (qrcode_getModule(&qrcode, x, y)) {
          tft.fillRect(offset_x + (x * scale), offset_y + (y * scale), scale,
                       scale, TFT_BLACK);
        }
      }
    }
  }

  void updateTrackInfo(const char *title, const char *artist) {
    _currentTitle = title;
    _currentArtist = artist;
    _scrollOffset = 0;

    // Calculate title width using Font 4
    spr.setTextFont(4);
    _titleWidth = spr.textWidth(_currentTitle);

    renderBottomSection(_lastProgress);
  }

  void loop() {
    if (_titleWidth > 300) {
      if (millis() - _lastScrollTime > 35) { // Fluid motion speed
        _scrollOffset += 2;
        if (_scrollOffset >= _titleWidth - 280) {
          _scrollOffset = 0;
        }
        renderBottomSection(_lastProgress);
        _lastScrollTime = millis();
      }
    }
  }

  void renderBottomSection(float progress) {
    _lastProgress = progress;
    spr.fillSprite(TFT_BLACK);

    // 1. Progress Bar
    spr.fillRect(10, 0, 300, 8, 0x3186);                   // Dark Grey
    spr.fillRect(10, 0, (int)(300 * progress), 8, 0x1DCA); // Spotify Green

    // 2. Track Title (Scroll Area)
    spr.setTextFont(4);
    spr.setTextColor(TFT_WHITE);
    spr.setTextWrap(false); // Force single line for scrolling
    if (_titleWidth > 300) {
      // Height increased to 40 to prevent "black box" clipping
      spr.setViewport(10, 15, 300, 40);
      spr.setCursor(-_scrollOffset, 0);
      spr.print(_currentTitle);
      spr.resetViewport();
    } else {
      spr.setCursor(10, 15);
      spr.print(_currentTitle);
    }

    // 3. Artist
    spr.setTextColor(0xBDF7);
    spr.setTextFont(2);
    spr.setCursor(10, 45); // Adjusted Y
    spr.print(_currentArtist);

    spr.pushSprite(0, 175);
  }

  void drawButtons(bool isPlaying) {
    _isPlaying = isPlaying;
    // Center-right area for buttons
    int bx = 170;
    int by = 90;
    tft.fillRect(bx, by - 30, 145, 60, TFT_BLACK);

    // Prev Triangle
    tft.fillTriangle(bx + 30, by - 20, bx + 30, by + 20, bx + 5, by, TFT_WHITE);

    // Play/Pause
    if (isPlaying) {
      tft.fillRect(bx + 55, by - 15, 8, 30, TFT_WHITE);
      tft.fillRect(bx + 75, by - 15, 8, 30, TFT_WHITE);
    } else {
      tft.fillTriangle(bx + 55, by - 20, bx + 55, by + 20, bx + 85, by,
                       TFT_WHITE);
    }

    // Next Triangle
    tft.fillTriangle(bx + 110, by - 20, bx + 110, by + 20, bx + 135, by,
                     TFT_WHITE);
  }

  void updateProgress(float progress) { renderBottomSection(progress); }

  void drawClock() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
      char timeStringBuff[10];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);

      tft.fillRect(10, 2, 80, 25, TFT_BLACK);
      tft.setTextFont(2);
      tft.setTextSize(1); // Ensure size is 1 for clean scaling
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(10, 5);
      tft.print(timeStringBuff);
    }
  }

  void drawWiFi(bool connected) {
    int x = 255;
    int y = 18;
    tft.fillRect(x - 15, 2, 40, 22, TFT_BLACK);
    if (connected) {
      tft.fillCircle(x, y, 2, TFT_WHITE);          // Dot
      tft.drawCircleHelper(x, y, 7, 1, TFT_WHITE); // Arc 1
      tft.drawCircleHelper(x, y, 7, 2, TFT_WHITE);
      tft.drawCircleHelper(x, y, 12, 1, TFT_WHITE); // Arc 2
      tft.drawCircleHelper(x, y, 12, 2, TFT_WHITE);
    } else {
      tft.drawLine(x - 10, 5, x + 10, 20, TFT_RED);
      tft.drawLine(x + 10, 5, x - 10, 20, TFT_RED);
    }
  }

  void drawSettingsButton() {
    // Small cog icon, moved left to x=285
    int x = 285;
    int y = 5;
    tft.fillRect(x - 2, y - 2, 20, 20, TFT_BLACK);
    tft.drawCircle(x + 8, y + 8, 4, TFT_DARKGREY);
    for (int i = 0; i < 8; i++) {
      float angle = i * 45 * PI / 180;
      tft.drawLine(x + 8, y + 8, x + 8 + cos(angle) * 7, y + 8 + sin(angle) * 7,
                   TFT_DARKGREY);
    }
  }

  void drawSettingsPage(int currentRotation, int currentBrightness) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.println("Settings");

    tft.setTextFont(2);
    tft.setCursor(10, 50);
    tft.println("Rotation:");
    // Options 0, 1, 2, 3
    const char *rotLabels[] = {"P1", "L1", "P2", "L2"};
    for (int i = 0; i < 4; i++) {
      uint16_t color = (i == currentRotation) ? 0x1DCA : TFT_DARKGREY;
      tft.fillRect(10 + i * 55, 75, 50, 30, color);
      tft.setCursor(20 + i * 55, 82);
      tft.setTextColor(TFT_WHITE);
      tft.print(rotLabels[i]);
    }

    tft.setCursor(10, 120);
    tft.println("Brightness:");
    for (int i = 1; i <= 5; i++) {
      uint16_t color = (i == currentBrightness) ? 0x1DCA : TFT_DARKGREY;
      tft.fillRect(10 + (i - 1) * 55, 145, 50, 30, color);
      tft.setCursor(30 + (i - 1) * 55, 152);
      tft.setTextColor(TFT_WHITE);
      tft.print(i);
    }

    tft.fillRect(110, 200, 100, 35, 0x1DCA);
    tft.setCursor(140, 210);
    tft.print("SAVE");
  }

  void drawArtwork(const char *url) {
    WiFiClientSecure sslClient;
    sslClient.setInsecure();
    HTTPClient http;
    http.begin(sslClient, url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      int len = http.getSize();
      uint8_t *buffer = (uint8_t *)malloc(len);
      if (buffer) {
        WiFiClient *stream = http.getStreamPtr();
        int read = 0;
        while (read < len) {
          if (stream->available()) {
            buffer[read++] = stream->read();
          }
        }
        if (jpeg.openRAM(buffer, len, jpegDraw)) {
          tft.fillScreen(TFT_BLACK); // Final wipe to clear "Connecting..." text
          jpeg.setUserPointer(&tft);
          jpeg.decode(
              10, 25,
              JPEG_SCALE_HALF); // Moved down to make room for clock/wifi
          jpeg.close();

          // Immediately redraw UI to prevent popping
          drawClock();
          drawWiFi(WiFi.status() == WL_CONNECTED);
          drawSettingsButton();
          drawButtons(_isPlaying);
        }
        free(buffer);
      }
    }
    http.end();
  }
};

#endif
