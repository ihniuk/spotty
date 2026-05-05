#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "SpotifyHandler.h"
#include "CYD28_TouchscreenR.h"

// --- Global Objects ---
CYD28_TouchR ts = CYD28_TouchR(320, 240);
DisplayManager displayManager;
ConfigManager configManager;
SpotifyHandler spotifyHandler;

unsigned long lastUpdate = 0;
unsigned long currentUpdateInterval = 10000; // Start with 10s
unsigned long cooldownUntil = 0;
unsigned long currentCooldownWait = 60000;
bool newDataAvailable = false;
bool initialWipeDone = false;

struct SpotifyData {
    String trackName;
    String artists[3];
    int numArtists;
    String albumName;
    struct {
        String url;
    } albumImages[3];
    int numImages;
    bool isPlaying;
    unsigned long progressMs;
    unsigned long durationMs;
} lastKnownPlaying;

String currentArtworkUrl = "";
String currentTrack = "";
bool lastPlayState = false;
bool isSettingsOpen = false;
int tempRotation = 0;
int tempBrightness = 3;
int tempGmtOffset = 0;
int settingsPage = 0;
unsigned long lastClockUpdate = 0;

void spotifyCallback(CurrentlyPlaying data) {
    lastKnownPlaying.trackName = String(data.trackName);
    lastKnownPlaying.numArtists = data.numArtists;
    for (int i = 0; i < data.numArtists && i < 3; i++) {
        lastKnownPlaying.artists[i] = String(data.artists[i].artistName);
    }
    lastKnownPlaying.numImages = data.numImages;
    for (int i = 0; i < data.numImages && i < 3; i++) {
        lastKnownPlaying.albumImages[i].url = String(data.albumImages[i].url);
    }
    lastKnownPlaying.isPlaying = data.isPlaying;
    lastKnownPlaying.progressMs = data.progressMs;
    lastKnownPlaying.durationMs = data.durationMs;
    newDataAvailable = true;
}

void handleTouch() {
    if (ts.touched()) {
        CYD28_TS_Point p = ts.getPointScaled();
        int16_t x = p.x;
        int16_t y = p.y;
        
        int rotation = configManager.config.rotation;
        bool isPortrait = (rotation == 0 || rotation == 2);
        int screenWidth = isPortrait ? 240 : 320;
        int screenHeight = isPortrait ? 320 : 240;

        if (isSettingsOpen) {
            // 1. NEXT Button (Top Right)
            if (x > screenWidth - 85 && y < 45) {
                settingsPage = (settingsPage + 1) % 4;
                displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
                delay(300);
                return;
            }

            // 2. Page Specific Content
            if (settingsPage == 0) { // ROTATION
                if (y > 90 && y < 180) {
                    for(int i=0; i<4; i++) {
                        int btnW = (screenWidth - 40) / 2;
                        int bx = 10 + (i % 2) * (btnW + 10);
                        int by = 90 + (i / 2) * 45;
                        if (x > bx && x < bx + btnW && y > by && y < by + 35) {
                            tempRotation = i;
                            displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
                            delay(200);
                        }
                    }
                }
            } 
            else if (settingsPage == 1) { // BRIGHTNESS
                int bY = 110;
                if (y > bY && y < bY + 40) {
                    int btnW = (screenWidth - 30) / 5;
                    for(int i=1; i<=5; i++) {
                        if (x > 10 + (i-1)*btnW && x < 10 + i*btnW) {
                            tempBrightness = i;
                            displayManager.setBrightness(tempBrightness);
                            displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
                            delay(200);
                        }
                    }
                }
            }
            else if (settingsPage == 2) { // TIME
                if (y > 140 && y < 185) {
                    if (x > 20 && x < 100) { // - button
                        tempGmtOffset -= 3600;
                        displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
                        delay(200);
                    }
                    if (x > screenWidth - 100 && x < screenWidth - 20) { // + button
                        tempGmtOffset += 3600;
                        displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
                        delay(200);
                    }
                }
            }

            // 3. SAVE button (Bottom)
            int sY = screenHeight - 60;
            if (y > sY && x > (screenWidth-130)/2 && x < (screenWidth+130)/2) {
                configManager.config.rotation = tempRotation;
                configManager.config.brightness = tempBrightness;
                configManager.config.gmtOffset_sec = tempGmtOffset;
                configManager.saveConfig();
                isSettingsOpen = false;
                ESP.restart();
            }
            return;
        }

        // Settings button check: top right corner (normal view)
        if (y < 50 && x > (screenWidth - 60)) {
            isSettingsOpen = true;
            settingsPage = 0;
            tempRotation = configManager.config.rotation;
            tempBrightness = configManager.config.brightness;
            tempGmtOffset = configManager.config.gmtOffset_sec;
            displayManager.drawSettingsPage(settingsPage, tempRotation, tempBrightness, tempGmtOffset, WiFi.localIP().toString().c_str());
            delay(500);
            return;
        }

        // Playback buttons
        int bx, by;
        if (isPortrait) {
            bx = (screenWidth - 145) / 2;
            by = 210; // Match DisplayManager
        } else {
            bx = 170;
            by = 90;
        }

        if (y > by - 35 && y < by + 35) {
            if (x > bx && x < bx + 45) {
                displayManager.drawButtons(lastKnownPlaying.isPlaying, 0); // Highlight Prev
                delay(150);
                spotifyHandler.prev();
                lastUpdate = 0;
            } else if (x > bx + 50 && x < bx + 95) {
                displayManager.drawButtons(lastKnownPlaying.isPlaying, 1); // Highlight Play
                delay(150);
                if (lastKnownPlaying.isPlaying) {
                    spotifyHandler.pause();
                } else {
                    spotifyHandler.play();
                }
                lastUpdate = 0;
            } else if (x > bx + 100 && x < bx + 145) {
                displayManager.drawButtons(lastKnownPlaying.isPlaying, 2); // Highlight Next
                delay(150);
                spotifyHandler.next();
                lastUpdate = 0;
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    configManager.loadConfig(); 
    displayManager.begin(configManager.config.rotation);
    displayManager.setBrightness(configManager.config.brightness); // Light up early!
    ts.begin();
    ts.setRotation(configManager.config.rotation); 
    ts.setThreshold(300); 

    // --- FACTORY RESET CHECK ---
    if (ts.touched()) {
        displayManager.drawResetConfirmation();
        Serial.println("Reset screen triggered. Wait for release...");
        while (ts.touched()) delay(10); 
        delay(500); // Small buffer
        
        bool choiceMade = false;
        int screenWidth = (configManager.config.rotation == 0 || configManager.config.rotation == 2) ? 240 : 320;
        
        while (!choiceMade) {
            if (ts.touched()) {
                CYD28_TS_Point p = ts.getPointScaled();
                Serial.printf("Reset Touch: X=%d, Y=%d\n", p.x, p.y);
                
                // YES hitbox (wider): x: 0-150, y: 130-220
                if (p.x > 0 && p.x < 150 && p.y > 130 && p.y < 220) {
                    Serial.println("Reset Confirmed!");
                    displayManager.showLoading("FACTORY RESET...");
                    delay(2000);
                    WiFiManager wm;
                    wm.resetSettings(); 
                    SPIFFS.format();    
                    ESP.restart();
                }
                // NO hitbox (wider): x: (screenWidth-150) to screenWidth, y: 130-220
                if (p.x > screenWidth - 150 && p.x < screenWidth && p.y > 130 && p.y < 220) {
                    Serial.println("Reset Cancelled.");
                    choiceMade = true;
                }
                delay(300);
            }
            delay(10);
        }
    }
    
    if (!configManager.loadConfig()) {
        configManager.setupPortal(displayManager);
    } else {
        displayManager.showLoading("Connecting to WiFi...");
        WiFiManager wm;
        if (!wm.autoConnect("Spotify-CYD-Setup")) {
            Serial.println("Failed to connect to WiFi!");
            delay(3000);
            ESP.restart();
        }
        // Apply saved GMT Offset
        configTime(configManager.config.gmtOffset_sec, 0, "pool.ntp.org"); 
    }

    spotifyHandler.begin(
        configManager.config.spotifyClientId,
        configManager.config.spotifyClientSecret,
        configManager.config.spotifyRefreshToken
    );
    
    displayManager.setBrightness(configManager.config.brightness);

    if (!spotifyHandler.authComplete) {
        String ip = WiFi.localIP().toString();
        String url = "http://" + ip;
        displayManager.showLoading("SCAN TO LOGIN");
        displayManager.showQRCode(url.c_str());
        Serial.println("Visit: " + url);
 
        spotifyHandler.handleAuth([](const char* token) {
            strcpy(configManager.config.spotifyRefreshToken, token);
            configManager.saveConfig();
            Serial.println("Token saved, restarting...");
            delay(2000);
            ESP.restart();
        });
    }
}

void loop() {
    if (!spotifyHandler.authComplete) {
        spotifyHandler.loop();
        return;
    }

    handleTouch();
    if (isSettingsOpen) return;
    displayManager.loop();

    // Initial wipe if WiFi is connected but we haven't cleared the screen yet
    if (WiFi.status() == WL_CONNECTED && !initialWipeDone) {
        displayManager.showLoading(""); // Clear "Connecting..." text
        initialWipeDone = true;
    }

    // Handle Rate Limit Cooldown
    if (millis() < cooldownUntil) {
        static unsigned long lastCountdownUpdate = 0;
        if (millis() - lastCountdownUpdate > 1000) {
            char buff[32];
            sprintf(buff, "Wait %lus...", (cooldownUntil - millis()) / 1000);
            displayManager.updateTrackInfo("Rate Limited", buff);
            lastCountdownUpdate = millis();
        }
        return; 
    }

    // Predictive Progress Animation
    if (lastKnownPlaying.isPlaying && !isSettingsOpen) {
        unsigned long timeSinceLastUpdate = millis() - lastUpdate;
        unsigned long predictedMs = lastKnownPlaying.progressMs + timeSinceLastUpdate;
        
        // Cap it so it doesn't go past 100% before the next poll
        if (predictedMs > lastKnownPlaying.durationMs) predictedMs = lastKnownPlaying.durationMs;
        
        float predictedProgress = (float)predictedMs / lastKnownPlaying.durationMs;
        displayManager.updateProgress(predictedProgress);
    }

    // Dynamic Polling Logic
    if (millis() - lastUpdate > currentUpdateInterval || lastUpdate == 0) {
        int result = spotifyHandler.spotify.getCurrentlyPlaying(spotifyCallback);
        lastUpdate = millis();

        if (result == 200 && newDataAvailable) {
            // Determine next interval based on state
            if (!lastKnownPlaying.isPlaying) {
                currentUpdateInterval = 30000; // 30s if paused
            } else {
                long remainingMs = lastKnownPlaying.durationMs - lastKnownPlaying.progressMs;
                if (remainingMs < 10000) {
                    currentUpdateInterval = 3000; // 3s if near end
                } else {
                    currentUpdateInterval = 10000; // 10s normal playing
                }
            }

            if (lastKnownPlaying.trackName != currentTrack) {
                displayManager.updateTrackInfo(lastKnownPlaying.trackName.c_str(), lastKnownPlaying.artists[0].c_str());
                currentTrack = lastKnownPlaying.trackName;
            }

            if (lastKnownPlaying.numImages > 1) {
                if (lastKnownPlaying.albumImages[1].url != currentArtworkUrl) {
                    displayManager.drawArtwork(lastKnownPlaying.albumImages[1].url.c_str());
                    currentArtworkUrl = lastKnownPlaying.albumImages[1].url;
                }
            }

            if (lastKnownPlaying.isPlaying != lastPlayState || currentTrack != lastKnownPlaying.trackName) {
                displayManager.drawButtons(lastKnownPlaying.isPlaying);
                lastPlayState = lastKnownPlaying.isPlaying;
            }

            // Immediate update for the progress bar
            float progress = (float)lastKnownPlaying.progressMs / lastKnownPlaying.durationMs;
            displayManager.updateProgress(progress);
            
            displayManager.drawWiFi(WiFi.status() == WL_CONNECTED);
            displayManager.drawSettingsButton();
            displayManager.drawClock();
            
            newDataAvailable = false;
            currentCooldownWait = 60000;
        } else if (result == 204) {
            currentUpdateInterval = 30000; // Check every 30s if nothing is playing
            if (currentTrack != "No Music") {
                displayManager.updateTrackInfo("No Music", "Playing");
                currentTrack = "No Music";
            }
            displayManager.drawWiFi(WiFi.status() == WL_CONNECTED);
            displayManager.drawSettingsButton();
            displayManager.drawClock();
            currentCooldownWait = 60000;
        } else if (result == 429) {
            Serial.print("Rate limited! Backing off...");
            cooldownUntil = millis() + currentCooldownWait;
            currentCooldownWait *= 2; 
            if (currentCooldownWait > 300000) currentCooldownWait = 300000;
        }
    }
}
