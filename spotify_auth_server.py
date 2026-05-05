import webbrowser
import http.server
import socketserver
import urllib.parse
import urllib.request
import base64
import json

# --- CONFIGURATION ---
PORT = 8888
REDIRECT_URI = f'http://localhost:{PORT}/callback'
SCOPES = 'user-read-playback-state user-modify-playback-state user-read-currently-playing'

class SpotifyAuthHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/callback'):
            query = urllib.parse.urlparse(self.path).query
            params = urllib.parse.parse_qs(query)
            
            if 'code' in params:
                code = params['code'][0]
                self.exchange_code(code)
            else:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Error: No code received from Spotify.")
        else:
            self.send_response(404)
            self.end_headers()

    def exchange_code(self, code):
        print(f"\n[INFO] Exchanging code for Refresh Token...")
        
        auth_header = base64.b64encode(f"{CLIENT_ID}:{CLIENT_SECRET}".encode()).decode()
        
        data = urllib.parse.urlencode({
            'grant_type': 'authorization_code',
            'code': code,
            'redirect_uri': REDIRECT_URI
        }).encode()
        
        req = urllib.request.Request(
            'https://accounts.spotify.com/api/token',
            data=data,
            headers={
                'Authorization': f'Basic {auth_header}',
                'Content-Type': 'application/x-www-form-urlencoded'
            }
        )
        
        try:
            with urllib.request.urlopen(req) as response:
                res_data = json.loads(response.read().decode())
                
                if 'refresh_token' in res_data:
                    token = res_data['refresh_token']
                    self.send_response(200)
                    self.send_header('Content-type', 'text/html')
                    self.end_headers()
                    
                    html = f"""
                    <html>
                        <body style="font-family: sans-serif; background: #121212; color: white; padding: 50px; text-align: center;">
                            <h1 style="color: #1DB954;">Success!</h1>
                            <p>Your Spotify Refresh Token is:</p>
                            <div style="background: #333; padding: 20px; border-radius: 10px; margin: 20px 0;">
                                <code style="font-size: 1.4em; color: #1DB954; word-break: break-all;">{token}</code>
                            </div>
                            <p>Copy this into your ESP32 WiFi Setup page.</p>
                            <p><small>You can now close this window and stop the Python script.</small></p>
                        </body>
                    </html>
                    """
                    self.wfile.write(html.encode())
                    print(f"\n[SUCCESS] Refresh Token: {token}")
                else:
                    self.wfile.write(b"Error: Refresh token not found in response.")
        except Exception as e:
            print(f"[ERROR] {e}")
            self.send_response(500)
            self.end_headers()
            self.wfile.write(f"Server Error: {e}".encode())

# --- MAIN ---
print("--- Spotify Local Auth Tool (100% Private) ---")
CLIENT_ID = input("Enter Spotify Client ID: ").strip()
CLIENT_SECRET = input("Enter Spotify Client Secret: ").strip()

auth_url = (
    "https://accounts.spotify.com/authorize"
    f"?client_id={CLIENT_ID}"
    "&response_type=code"
    f"&redirect_uri={urllib.parse.quote(REDIRECT_URI)}"
    f"&scope={urllib.parse.quote(SCOPES)}"
)

print(f"\n1. Ensure 'http://localhost:{PORT}/callback' is in your Spotify Redirect URIs.")
print(f"2. Opening browser to: {auth_url}")
webbrowser.open(auth_url)

with socketserver.TCPServer(("", PORT), SpotifyAuthHandler) as httpd:
    print("\n[WAITING] Waiting for Spotify to redirect...")
    httpd.handle_request()
