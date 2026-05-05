#ifndef SPOTIFY_HANDLER_H
#define SPOTIFY_HANDLER_H

#include <WiFiClientSecure.h>
#include <SpotifyArduino.h>
#include <SpotifyArduinoCert.h>
#include <WebServer.h>

class SpotifyHandler {
public:
    WiFiClientSecure client;
    SpotifyArduino spotify;
    WebServer server;
    
    char* clientId;
    char* clientSecret;
    char* refreshToken;
    bool authComplete = false;
    void (*onTokenReceived)(const char*);

    SpotifyHandler() : spotify(client, "", ""), server(80) {
        client.setInsecure(); // Bypass potentially expired root CA certs
    }

    void begin(char* cId, char* cSec, char* rTok) {
        clientId = cId;
        clientSecret = cSec;
        refreshToken = rTok;
        spotify.lateInit(clientId, clientSecret, refreshToken);
        if (strlen(refreshToken) > 0) {
            authComplete = true;
        }
    }

    void handleCallback() {
        if (server.hasArg("url")) {
            String url = server.arg("url");
            String code = "";
            
            int codeStart = url.indexOf("code=");
            int codeStartEncoded = url.indexOf("code%3D");

            if (codeStart != -1) {
                code = url.substring(codeStart + 5);
            } else if (codeStartEncoded != -1) {
                code = url.substring(codeStartEncoded + 7);
            } else if (!url.startsWith("http")) {
                code = url;
            }

            if (code != "") {
                int codeEnd = code.indexOf("&");
                if (codeEnd == -1) codeEnd = code.indexOf("%26");
                if (codeEnd != -1) code = code.substring(0, codeEnd);
                
                Serial.println("Code captured! Exchanging...");
                
                String dummyRedirect = "https://google.com/";
                String token = spotify.requestAccessTokens(code.c_str(), dummyRedirect.c_str());
                
                if (token != "") {
                    if (onTokenReceived) onTokenReceived(token.c_str());
                    server.send(200, "text/html", "<h1>Success!</h1><p>Token saved. Device restarting...</p>");
                    return;
                } else {
                    server.send(200, "text/html", "<h1>Exchange Failed!</h1><p>Extracted code: " + code.substring(0, 10) + "...<br><br>But Spotify rejected the trade. It likely <b>expired</b> (you only have 60 seconds) or there is an SSL issue. Try generating a fresh one!</p>");
                    return;
                }
            }
        }
        server.send(200, "text/html", "<h1>Parse Error</h1><p>Could not extract the code from what you pasted. Ensure it contains 'code='.</p>");
    }

    void handleRoot() {
        String dummyRedirect = "https%3A%2F%2Fgoogle.com%2F";
        String authUrl = "https://accounts.spotify.com/authorize?client_id=" + String(clientId) + 
                         "&response_type=code&redirect_uri=" + dummyRedirect +
                         "&scope=user-read-playback-state%20user-modify-playback-state%20user-read-currently-playing";

        String html = "<html><body style='text-align:center;padding:30px;font-family:sans-serif;background:#121212;color:white;'>"
                      "<h1 style='color:#1DB954;'>Spotify Local Auth</h1>"
                      "<p>1. Ensure <b>https://google.com/</b> is in your Spotify Dashboard.</p>"
                      "<a href='" + authUrl + "' target='_blank' style='display:inline-block;padding:15px;background:#1DB954;color:white;text-decoration:none;border-radius:30px;margin:20px;font-weight:bold;'>Step 1: Login with Spotify</a>"
                      "<p>2. You will be redirected to Google. <b>Copy that URL</b>.</p>"
                      "<form action='/callback/' method='GET'>"
                      "<input type='text' name='url' placeholder='Paste the Google URL here' style='width:90%;padding:10px;border-radius:5px;border:none;'><br><br>"
                      "<button type='submit' style='padding:10px 30px;background:#1DB954;color:white;border:none;border-radius:20px;font-weight:bold;'>Step 2: Save Token</button>"
                      "</form></body></html>";
        server.send(200, "text/html", html);
    }

    void handleAuth(void (*callback)(const char*)) {
        onTokenReceived = callback;
        server.on("/", [this]() { handleRoot(); });
        server.on("/callback/", [this]() { handleCallback(); });
        server.begin();
    }

    void loop() {
        server.handleClient();
    }

    void next() { spotify.nextTrack(); }
    void prev() { spotify.previousTrack(); }
    void pause() { spotify.pause(); }
    void play() { spotify.play(); }
};

#endif
