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
  bool _isPortrait = false;
  int _screenWidth = 320;
  int _screenHeight = 240;

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
    _isPortrait = (rotation == 0 || rotation == 2);
    _screenWidth = _isPortrait ? 240 : 320;
    _screenHeight = _isPortrait ? 320 : 240;
    
    tft.invertDisplay(true);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Backlight setup
    ledcSetup(0, 5000, 8);
    ledcAttachPin(21, 0);

    // Sprite width matches screen width
    if (spr.created()) spr.deleteSprite();
    spr.createSprite(_screenWidth, 65);
  }

  void setBrightness(int level) {
    int val = map(level, 1, 5, 20, 255);
    ledcWrite(0, val);
  }

  void showLoading(const char *msg) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 20);
    tft.setTextSize(2);
    tft.println(msg);
  }

  void showQRCode(const char *text) {
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, text);

    int scale = 4;
    int offset_x = (_screenWidth - (qrcode.size * scale)) / 2;
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
    spr.setTextFont(4);
    _titleWidth = spr.textWidth(_currentTitle);
    renderBottomSection(_lastProgress);
  }

  void loop() {
    int scrollThreshold = _screenWidth - 20;
    if (_titleWidth > scrollThreshold) {
      if (millis() - _lastScrollTime > 35) {
        _scrollOffset += 2;
        if (_scrollOffset >= _titleWidth - (scrollThreshold - 20)) {
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

    int barWidth = _screenWidth - 20;
    spr.fillRect(10, 0, barWidth, 8, 0x3186);
    spr.fillRect(10, 0, (int)(barWidth * progress), 8, 0x1DCA);

    spr.setTextFont(4);
    spr.setTextColor(TFT_WHITE);
    spr.setTextWrap(false);
    if (_titleWidth > barWidth) {
      spr.setViewport(10, 15, barWidth, 40);
      spr.setCursor(-_scrollOffset, 0);
      spr.print(_currentTitle);
      spr.resetViewport();
    } else {
      spr.setCursor(10, 15);
      spr.print(_currentTitle);
    }

    spr.setTextColor(0xBDF7);
    spr.setTextFont(2);
    spr.setCursor(10, 45);
    spr.print(_currentArtist);

    spr.pushSprite(0, _screenHeight - 65);
  }

  void drawButtons(bool isPlaying, int highlightedButton = -1) {
    _isPlaying = isPlaying;
    int bx, by;
    if (_isPortrait) {
      bx = (_screenWidth - 145) / 2;
      by = 210; // Moved up from 245 to avoid progress bar
    } else {
      bx = 170;
      by = 90;
    }
    
    tft.fillRect(bx, by - 30, 145, 60, TFT_BLACK);

    // Prev
    uint16_t c1 = (highlightedButton == 0) ? TFT_DARKGREY : TFT_WHITE;
    tft.fillTriangle(bx + 30, by - 20, bx + 30, by + 20, bx + 5, by, c1);
    
    // Play/Pause
    uint16_t c2 = (highlightedButton == 1) ? TFT_DARKGREY : TFT_WHITE;
    if (isPlaying) {
      tft.fillRect(bx + 55, by - 15, 8, 30, c2);
      tft.fillRect(bx + 75, by - 15, 8, 30, c2);
    } else {
      tft.fillTriangle(bx + 55, by - 20, bx + 55, by + 20, bx + 85, by, c2);
    }
    
    // Next
    uint16_t c3 = (highlightedButton == 2) ? TFT_DARKGREY : TFT_WHITE;
    tft.fillTriangle(bx + 110, by - 20, bx + 110, by + 20, bx + 135, by, c3);
  }

  void updateProgress(float progress) { renderBottomSection(progress); }

  void drawClock() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
      char timeStringBuff[10];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
      tft.fillRect(5, 2, 80, 25, TFT_BLACK);
      tft.setTextFont(2);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(10, 5);
      tft.print(timeStringBuff);
    }
  }

  void drawWiFi(bool connected) {
    int x = _screenWidth - 65;
    int y = 18;
    tft.fillRect(x - 15, 2, 40, 22, TFT_BLACK);
    if (connected) {
      tft.fillCircle(x, y, 2, TFT_WHITE);
      tft.drawCircleHelper(x, y, 7, 1, TFT_WHITE);
      tft.drawCircleHelper(x, y, 7, 2, TFT_WHITE);
      tft.drawCircleHelper(x, y, 12, 1, TFT_WHITE);
      tft.drawCircleHelper(x, y, 12, 2, TFT_WHITE);
    } else {
      tft.drawLine(x - 10, 5, x + 10, 20, TFT_RED);
      tft.drawLine(x + 10, 5, x - 10, 20, TFT_RED);
    }
  }

  void drawSettingsButton() {
    int x = _screenWidth - 35;
    int y = 5;
    tft.fillRect(x, y, 30, 30, TFT_BLACK); // Background

    // Three horizontal lines with "knobs" (Modern Sliders Look)
    for (int i = 0; i < 3; i++) {
      int rowY = y + 6 + (i * 8);
      tft.drawFastHLine(x + 2, rowY, 26, TFT_DARKGREY);
      
      // Offset the knobs for a "tuned" look
      int knobX = (i == 1) ? x + 18 : x + 8; 
      tft.fillRect(knobX, rowY - 2, 6, 5, TFT_WHITE);
    }
  }

  void drawResetConfirmation() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM); // Middle Center
    
    tft.drawString("FACTORY RESET?", _screenWidth / 2, 60);
    tft.drawString("This will wipe all settings.", _screenWidth / 2, 90);

    // YES Button (Red)
    tft.fillRect(20, 140, 120, 60, TFT_RED);
    tft.drawString("YES", 80, 170);

    // NO Button (Greenish)
    tft.fillRect(_screenWidth - 140, 140, 120, 60, 0x1DCA);
    tft.drawString("NO", _screenWidth - 80, 170);
    
    tft.setTextDatum(TL_DATUM); // Reset to Top Left
  }

  void drawSettingsPage(int page, int currentRotation, int currentBrightness, int currentGmtOffset, const char* ip) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    
    // Header with Page indicator
    char header[32];
    sprintf(header, "Settings %d/4", page + 1);
    tft.println(header);

    // Next Page Button (Top Right)
    tft.fillRect(_screenWidth - 80, 5, 75, 35, TFT_DARKGREY);
    tft.setCursor(_screenWidth - 70, 12);
    tft.setTextFont(2);
    tft.print("NEXT >");

    if (page == 0) {
      // --- PAGE 1: ROTATION ---
      tft.setTextFont(2);
      tft.setCursor(10, 60);
      tft.println("Screen Rotation:");
      const char *rotLabels[] = {"P1", "L1", "P2", "L2"};
      for (int i = 0; i < 4; i++) {
        uint16_t color = (i == currentRotation) ? 0x1DCA : TFT_DARKCYAN;
        int btnW = (_screenWidth - 40) / 2;
        int x = 10 + (i % 2) * (btnW + 10);
        int y = 90 + (i / 2) * 45;
        tft.fillRect(x, y, btnW, 35, color);
        tft.setCursor(x + (btnW/2) - 10, y + 10);
        tft.setTextColor(TFT_WHITE);
        tft.print(rotLabels[i]);
      }
    } 
    else if (page == 1) {
      // --- PAGE 2: BRIGHTNESS ---
      tft.setTextFont(2);
      tft.setCursor(10, 80);
      tft.println("Screen Brightness:");
      for (int i = 1; i <= 5; i++) {
        uint16_t color = (i == currentBrightness) ? 0x1DCA : TFT_DARKCYAN;
        int btnW = (_screenWidth - 30) / 5;
        int x = 10 + (i - 1) * (btnW + 2);
        int y = 110;
        tft.fillRect(x, y, btnW, 40, color);
        tft.setCursor(x + (btnW/2) - 4, y + 12);
        tft.print(i);
      }
    }
    else if (page == 2) {
      // --- PAGE 3: TIME ---
      tft.setTextFont(2);
      tft.setCursor(10, 70);
      tft.println("Time Zone (GMT Offset):");
      
      tft.setTextFont(4);
      tft.setCursor(_screenWidth / 2 - 40, 105);
      char gmtStr[16];
      sprintf(gmtStr, "%+d hrs", currentGmtOffset / 3600);
      tft.print(gmtStr);

      // - / + Buttons
      tft.fillRect(20, 140, 80, 45, TFT_RED);
      tft.setCursor(50, 152); tft.print("-");
      
      tft.fillRect(_screenWidth - 100, 140, 80, 45, 0x1DCA);
      tft.setCursor(_screenWidth - 70, 152); tft.print("+");
    }
    else if (page == 3) {
      // --- PAGE 4: INFO ---
      tft.setTextFont(2);
      tft.setCursor(10, 60);
      tft.setTextColor(0xBDF7);
      tft.print("IP Address: ");
      tft.setTextColor(TFT_WHITE);
      tft.println(ip);

      tft.setCursor(10, 90);
      tft.setTextColor(0xBDF7);
      tft.print("Software: ");
      tft.setTextColor(TFT_WHITE);
      tft.println("Spotty-CYD");

      tft.setCursor(10, 120);
      tft.setTextColor(0xBDF7);
      tft.print("Version: ");
      tft.setTextColor(TFT_WHITE);
      tft.println("v1.2.1");

      tft.setCursor(10, 150);
      tft.setTextColor(0xBDF7);
      tft.print("Build: ");
      tft.setTextColor(TFT_WHITE);
      tft.println(__DATE__);
    }

    // Save Button (Fixed at bottom)
    int saveY = _screenHeight - 55;
    tft.fillRect((_screenWidth - 120)/2, saveY, 120, 45, 0x1DCA);
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);
    tft.setCursor((_screenWidth - 120)/2 + 40, saveY + 15);
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
          tft.fillScreen(TFT_BLACK);
          jpeg.setUserPointer(&tft);
          
          int artX = _isPortrait ? (_screenWidth - 200) / 2 : 10;
          int artY = _isPortrait ? 35 : 25;
          
          jpeg.decode(artX, artY, JPEG_SCALE_HALF);
          jpeg.close();

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
