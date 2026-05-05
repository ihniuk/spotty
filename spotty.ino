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
unsigned long lastClockUpdate = 0;// --- Callback Function ---
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
        
        if (isSettingsOpen) {
            // Rotation buttons: y around 75-105
            if (y > 75 && y < 105) {
                for(int i=0; i<4; i++) {
                    if (x > 10 + i*55 && x < 60 + i*55) {
                        tempRotation = i;
                        displayManager.drawSettingsPage(tempRotation, tempBrightness);
                        delay(200);
                    }
                }
            }
            // Brightness buttons: y around 145-175
            else if (y > 145 && y < 175) {
                for(int i=1; i<=5; i++) {
                    if (x > 10 + (i-1)*55 && x < 60 + (i-1)*55) {
                        tempBrightness = i;
                        displayManager.setBrightness(tempBrightness);
                        displayManager.drawSettingsPage(tempRotation, tempBrightness);
                        delay(200);
                    }
                }
            }
            // SAVE button: y around 200-235
            else if (y > 200 && y < 235 && x > 110 && x < 210) {
                configManager.config.rotation = tempRotation;
                configManager.config.brightness = tempBrightness;
                configManager.saveConfig();
                isSettingsOpen = false;
                ESP.restart(); // Restart to apply rotation
            }
            return;
        }

        // Settings button check: top right
        if (y < 30 && x > 280) {
            isSettingsOpen = true;
            tempRotation = configManager.config.rotation;
            tempBrightness = configManager.config.brightness;
            displayManager.drawSettingsPage(tempRotation, tempBrightness);
            delay(500);
            return;
        }

        if (y > 70 && y < 130) {
            if (x > 165 && x < 210) {
                spotifyHandler.prev();
                lastUpdate = 0;
            } else if (x > 220 && x < 270) {
                if (lastKnownPlaying.isPlaying) {
                    spotifyHandler.pause();
                } else {
                    spotifyHandler.play();
                }
                lastUpdate = 0;
            } else if (x > 275 && x < 320) {
                spotifyHandler.next();
                lastUpdate = 0;
            }
        }
        delay(300);
    }
}

void setup() {
    Serial.begin(115200);
    configManager.loadConfig(); // Load early to get rotation
    displayManager.begin(configManager.config.rotation);
    ts.begin();

    // --- FACTORY RESET CHECK ---
    // If screen is touched during the first 2 seconds of boot, wipe everything.
    if (ts.touched()) {
        displayManager.showLoading("FACTORY RESET...\nRelease screen!");
        delay(3000);
        WiFiManager wm;
        wm.resetSettings(); // Erase WiFi
        SPIFFS.format();    // Erase Config
        Serial.println("Factory Reset Complete!");
        ESP.restart();
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
        configTime(0, 0, "pool.ntp.org"); // Initialize time for the clock
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
    displayManager.loop();

    if (isSettingsOpen) return;

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
