#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "DisplayManager.h"

class ConfigManager {
public:
    struct Config {
        char spotifyClientId[64];
        char spotifyClientSecret[64];
        char spotifyRefreshToken[160];
        int rotation;
        int brightness;
    } config;

    ConfigManager() {
        memset(&config, 0, sizeof(config));
        config.rotation = 1;
        config.brightness = 3;
    }

    bool loadConfig() {
        if (!SPIFFS.begin(true)) return false;
        if (!SPIFFS.exists("/config.json")) return false;

        fs::File configFile = SPIFFS.open("/config.json", "r");
        if (!configFile) return false;

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, configFile);
        configFile.close();

        if (error) return false;

        strlcpy(config.spotifyClientId, doc["spotifyClientId"] | "", sizeof(config.spotifyClientId));
        strlcpy(config.spotifyClientSecret, doc["spotifyClientSecret"] | "", sizeof(config.spotifyClientSecret));
        strlcpy(config.spotifyRefreshToken, doc["spotifyRefreshToken"] | "", sizeof(config.spotifyRefreshToken));
        config.rotation = doc["rotation"] | 1;
        config.brightness = doc["brightness"] | 3;

        return true;
    }

    bool saveConfig() {
        StaticJsonDocument<512> doc;
        doc["spotifyClientId"] = config.spotifyClientId;
        doc["spotifyClientSecret"] = config.spotifyClientSecret;
        doc["spotifyRefreshToken"] = config.spotifyRefreshToken;
        doc["rotation"] = config.rotation;
        doc["brightness"] = config.brightness;

        fs::File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) return false;

        serializeJson(doc, configFile);
        configFile.close();
        return true;
    }

    void setupPortal(DisplayManager &displayManager) {
        WiFiManager wm;
        
        WiFiManagerParameter custom_client_id("client_id", "Spotify Client ID", config.spotifyClientId, 64);
        WiFiManagerParameter custom_client_secret("client_secret", "Spotify Client Secret", config.spotifyClientSecret, 64);
        WiFiManagerParameter custom_refresh_token("refresh_token", "Spotify Refresh Token (Optional)", config.spotifyRefreshToken, 160);

        wm.addParameter(&custom_client_id);
        wm.addParameter(&custom_client_secret);
        wm.addParameter(&custom_refresh_token);

        displayManager.showLoading("Connect to:\nSpotify-CYD-Setup");

        if (!wm.autoConnect("Spotify-CYD-Setup")) {
            Serial.println("Failed to connect, restarting...");
            delay(3000);
            ESP.restart();
        }

        strlcpy(config.spotifyClientId, custom_client_id.getValue(), sizeof(config.spotifyClientId));
        strlcpy(config.spotifyClientSecret, custom_client_secret.getValue(), sizeof(config.spotifyClientSecret));
        strlcpy(config.spotifyRefreshToken, custom_refresh_token.getValue(), sizeof(config.spotifyRefreshToken));
        
        saveConfig();
        displayManager.showLoading("WiFi Connected!");
    }
};

#endif
