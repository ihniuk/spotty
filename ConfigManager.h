#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "DisplayManager.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <esp_partition.h>
#include <esp_spi_flash.h>

#define RAW_CONFIG_ADDR 0x3F0000
#define RAW_MAGIC "SPOT"

class ConfigManager {
public:
  struct Config {
    char wifiSsid[32];
    char wifiPass[64];
    char spotifyClientId[65];
    char spotifyClientSecret[65];
    char spotifyRefreshToken[512];
    int rotation;
    int brightness;
    long gmtOffset_sec;
  } config;

  ConfigManager() {
    memset(&config, 0, sizeof(config));
    config.rotation = 1;
    config.brightness = 3;
    config.gmtOffset_sec = 0;
  }

  bool loadConfig() {
    // 1. ALWAYS check the raw flash injection FIRST!
    // We must do this before SPIFFS.begin(true) because the SPIFFS format
    // will overwrite the 0x3F0000 memory space and destroy our payload.
    bool rawLoaded = loadFromRawFlash();

    // 2. Now it is safe to initialize SPIFFS (which will format if empty)
    if (SPIFFS.begin(true)) {
      if (rawLoaded) {
        // If we successfully injected, save it permanently to the new
        // filesystem
        saveConfig();
        return true;
      }

      // 3. Normal boot: check for existing config file
      if (SPIFFS.exists("/config.json")) {
        File file = SPIFFS.open("/config.json", "r");
        if (file) {
          StaticJsonDocument<1536> doc;
          DeserializationError error = deserializeJson(doc, file);
          file.close();
          if (!error) {
            copyDocToConfig(doc);
            return true;
          }
        }
      }
    }

    return rawLoaded;
  }

  bool loadFromRawFlash() {
    Serial.println("Attempting to load config from raw flash (0x3F0000)...");
    uint32_t magic_buf[2] = {0}; // Guaranteed 4-byte alignment
    if (spi_flash_read(RAW_CONFIG_ADDR, magic_buf, 4) != ESP_OK) {
      Serial.println("Error: spi_flash_read failed for magic bytes.");
      return false;
    }

    char *magic = (char *)magic_buf;
    Serial.print("Magic found: '");
    Serial.print(magic[0]);
    Serial.print(magic[1]);
    Serial.print(magic[2]);
    Serial.print(magic[3]);
    Serial.println("'");

    if (strncmp(magic, RAW_MAGIC, 4) == 0) {
      Serial.println("Magic matches! Reading JSON buffer...");
      uint32_t buffer_words[256] = {
          0}; // 1024 bytes, Guaranteed 4-byte alignment
      if (spi_flash_read(RAW_CONFIG_ADDR + 4, buffer_words, 1024) != ESP_OK) {
        Serial.println("Error: spi_flash_read failed for JSON buffer.");
        return false;
      }

      char *buffer = (char *)buffer_words;
      Serial.println("Raw JSON Buffer (first 200 chars):");
      for (int i = 0; i < 200; i++) {
        if (buffer[i] == 0)
          break;
        Serial.print(buffer[i]);
      }
      Serial.println();

      StaticJsonDocument<1536> doc;
      DeserializationError error = deserializeJson(doc, buffer);
      if (!error) {
        Serial.println("JSON parsed successfully! Injecting...");
        copyDocToConfig(doc);
        
        // Erase the magic header so it only injects once
        spi_flash_erase_sector(RAW_CONFIG_ADDR / 4096);
        
        return true;
      } else {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.println("Magic does not match 'SPOT'.");
    }
    return false;
  }

  void copyDocToConfig(StaticJsonDocument<1536> &doc) {
    if (doc.containsKey("ssid"))
      strlcpy(config.wifiSsid, doc["ssid"], sizeof(config.wifiSsid));
    if (doc.containsKey("pass"))
      strlcpy(config.wifiPass, doc["pass"], sizeof(config.wifiPass));
    if (doc.containsKey("clientId"))
      strlcpy(config.spotifyClientId, doc["clientId"],
              sizeof(config.spotifyClientId));
    if (doc.containsKey("clientSecret"))
      strlcpy(config.spotifyClientSecret, doc["clientSecret"],
              sizeof(config.spotifyClientSecret));
    if (doc.containsKey("refreshToken"))
      strlcpy(config.spotifyRefreshToken, doc["refreshToken"],
              sizeof(config.spotifyRefreshToken));
    if (doc.containsKey("rotation"))
      config.rotation = doc["rotation"];
    if (doc.containsKey("brightness"))
      config.brightness = doc["brightness"];
    if (doc.containsKey("gmt"))
      config.gmtOffset_sec = doc["gmt"];
  }

  bool saveConfig() {
    if (!SPIFFS.begin(true))
      return false;
    fs::File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
      return false;

    StaticJsonDocument<1536> doc;
    doc["ssid"] = config.wifiSsid;
    doc["pass"] = config.wifiPass;
    doc["clientId"] = config.spotifyClientId;
    doc["clientSecret"] = config.spotifyClientSecret;
    doc["refreshToken"] = config.spotifyRefreshToken;
    doc["rotation"] = config.rotation;
    doc["brightness"] = config.brightness;
    doc["gmt"] = config.gmtOffset_sec;

    serializeJson(doc, configFile);
    configFile.close();
    return true;
  }

  void setupPortal(DisplayManager &displayManager) {
    WiFiManager wm;

    WiFiManagerParameter custom_client_id("client_id", "Spotify Client ID",
                                          config.spotifyClientId, 64);
    WiFiManagerParameter custom_client_secret("client_secret",
                                              "Spotify Client Secret",
                                              config.spotifyClientSecret, 64);
    WiFiManagerParameter custom_refresh_token(
        "refresh_token", "Spotify Refresh Token (Optional)",
        config.spotifyRefreshToken, 160);

    wm.addParameter(&custom_client_id);
    wm.addParameter(&custom_client_secret);
    wm.addParameter(&custom_refresh_token);

    displayManager.showLoading(
        "Connect to:\nSpotify-CYD-Setup\n\nhttp://192.168.1.4");

    if (!wm.autoConnect("Spotify-CYD-Setup")) {
      Serial.println("Failed to connect, restarting...");
      delay(3000);
      ESP.restart();
    }

    strlcpy(config.spotifyClientId, custom_client_id.getValue(),
            sizeof(config.spotifyClientId));
    strlcpy(config.spotifyClientSecret, custom_client_secret.getValue(),
            sizeof(config.spotifyClientSecret));
    strlcpy(config.spotifyRefreshToken, custom_refresh_token.getValue(),
            sizeof(config.spotifyRefreshToken));

    saveConfig();
    displayManager.showLoading("WiFi Connected!");
  }
};

#endif
