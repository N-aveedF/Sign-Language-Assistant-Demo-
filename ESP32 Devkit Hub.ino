// ==========================================
// BLYNK CONFIGURATION (MUST BE AT THE VERY TOP)
// ==========================================
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>

// ==========================================
// WI-FI CONFIGURATION
// ==========================================
const char* ssid = "YOUR_WIFI_NETWORK_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Store the latest sign globally so the web page can fetch it
String currentDetectedSign = "WAITING";

// ==========================================
// CLEAN, TEXT-ONLY WEBPAGE (PROGMEM)
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sign Language HUD</title>
    <script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="flex flex-col items-center justify-center min-h-screen bg-gray-100 p-4 font-sans text-gray-800">
    <div class="w-full max-w-md bg-white rounded-3xl shadow-xl overflow-hidden border border-gray-200">
        <div class="bg-blue-600 p-6 text-center text-white">
            <h1 class="text-2xl font-bold mb-1">Sign Assistant HUD</h1>
            <div class="flex items-center justify-center space-x-2 text-sm opacity-90">
                <span id="status-dot" class="w-3 h-3 rounded-full bg-red-400 shadow"></span>
                <span id="status-text">Connecting...</span>
            </div>
        </div>

        <div class="p-8 text-center flex flex-col items-center justify-center min-h-[200px]">
            <h2 class="text-sm uppercase tracking-widest text-gray-400 font-semibold mb-2">Current Sign</h2>
            <p id="detected-sign" class="text-5xl font-extrabold text-blue-600 tracking-tight transition-all duration-300">WAITING</p>
        </div>
    </div>

    <script>
        // Use AJAX Polling to bypass firewall restrictions on strict Wi-Fi networks
        function updateSign() {
            fetch('/get_sign')
                .then(response => {
                    if (!response.ok) throw new Error('Network response was not ok');
                    return response.text();
                })
                .then(text => {
                    document.getElementById('status-dot').classList.replace('bg-red-400', 'bg-green-400');
                    document.getElementById('status-text').innerText = "Live & Listening";
                    
                    // Update text only if it has changed to prevent flickering
                    let signElement = document.getElementById('detected-sign');
                    if (signElement.innerText !== text) {
                        signElement.innerText = text;
                    }
                })
                .catch(error => {
                    document.getElementById('status-dot').classList.replace('bg-green-400', 'bg-red-400');
                    document.getElementById('status-text').innerText = "Disconnected. Retrying...";
                });
        }

        window.addEventListener('load', () => {
            // Check for new signs twice a second (500ms)
            setInterval(updateSign, 500);
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // 1. Initialize the Physical LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Booting System..");

  // 2. Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // 3. Configure Blynk (Runs in background)
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  
  Serial.println("\n--- SYSTEM READY ---");
  Serial.print("Local Webpage IP: ");
  Serial.println(WiFi.localIP());

  // Wait a few seconds to show IP, then clear
  delay(4000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready.");
  lcd.setCursor(0, 1);
  lcd.print("Awaiting Sign...");

  // Push startup status to Blynk Cloud
  Blynk.virtualWrite(V0, "Online"); 
  Blynk.virtualWrite(V1, "Waiting for input");

  // 4. Setup Web Server Routing
  
  // Route to load the main webpage
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });

  // Route for the webpage to ask "what is the current sign?"
  server.on("/get_sign", HTTP_GET, []() {
    server.send(200, "text/plain", currentDetectedSign);
  });

  // Route where the Python AI sends the detected signs
  server.on("/ai_input", HTTP_POST, []() {
    if (server.hasArg("sign")) {
      String detectedSign = server.arg("sign");
      Serial.println("AI Detected: " + detectedSign);
      
      // Update global variable for the Web Page
      currentDetectedSign = detectedSign;
      
      // Update the Physical LCD Display instantly
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Detected Sign:");
      lcd.setCursor(0, 1);
      lcd.print("-> " + detectedSign);

      // Push update to Blynk IoT Dashboard
      Blynk.virtualWrite(V0, "Active");       // Status Datastream
      Blynk.virtualWrite(V1, detectedSign);   // Sign Text Datastream

      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Missing sign argument");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();
  Blynk.run(); // Keep Blynk Cloud connection alive
}
