#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoOTA.h> // Добавляем OTA
#include <Update.h>     // Для обновлений

#define LED_PIN 8
const char *ap_ssid_base = "192.168.4.1";
const char *ap_password = "12345678";

Preferences prefs;
String wifi_ssid;
String wifi_pass;
bool wifi_connected = false;

WebServer server(80);

void handleRoot()
{
  IPAddress ap_ip = WiFi.softAPIP();
  String html = "<h1 style='font-size:1.5em;'>ESP32-C3 Access Point Info</h1>";
  html += "<p><b>AP SSID:</b> " + String(WiFi.softAPSSID()) + "</p>";
  html += "<p><b>AP IP:</b> " + ap_ip.toString() + "</p>";
  html += "<p>Connect to this WiFi and open <a href='http://" + ap_ip.toString() + "'>http://" + ap_ip.toString() + "</a> in your browser.</p>";
  html += "<p><a href='/editor'>📝 Code Editor</a> | <a href='/ota'>🔄 OTA Update</a></p>";
  server.send(200, "text/html", html);
}

void handleCodeEditor()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-C3 Code Editor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 10px; background: #f9f9f9; }
        h1 { font-size: 1.2em; }
        form, .snippets, .console { margin-bottom: 20px; }
        textarea { width: 100%; min-height: 200px; font-family: monospace; font-size: 1em; box-sizing: border-box; }
        button { padding: 12px 20px; margin: 5px 2px; font-size: 1em; border-radius: 6px; border: none; background: #007bff; color: #fff; }
        button:active { background: #0056b3; }
        input[type="file"] { font-size: 1em; }
        .console { background: #222; color: #0f0; padding: 10px; height: 150px; overflow-y: auto; border-radius: 6px; font-size: 0.95em; }
        @media (max-width: 600px) {
            h1 { font-size: 1em; }
            button { width: 100%; margin: 5px 0; }
            .console { height: 120px; font-size: 0.9em; }
        }
    </style>
</head>
<body>
    <h1>📝 ESP32-C3 Code Editor</h1>
    <form action="/upload" method="post" enctype="multipart/form-data">
        <h3>Upload New Firmware:</h3>
        <input type="file" name="firmware" accept=".bin">
        <button type="submit">Upload Firmware</button>
    </form>
    
    <div class="snippets">
        <h3>Quick Code Snippets:</h3>
        <button onclick="sendCommand('blink_fast')">Fast Blink</button>
        <button onclick="sendCommand('blink_slow')">Slow Blink</button>
        <button onclick="sendCommand('led_on')">LED ON</button>
        <button onclick="sendCommand('led_off')">LED OFF</button>
        <button onclick="sendCommand('restart')">Restart ESP</button>
    </div>

    <h3>Console Output:</h3>
    <div id="console" class="console"></div>

    <script>
        function sendCommand(cmd) {
            fetch("/cmd?action=" + cmd)
                .then(response => response.text())
                .then(data => {
                    document.getElementById("console").innerHTML += "&gt; " + cmd + ": " + data + "<br>";
                    document.getElementById("console").scrollTop = document.getElementById("console").scrollHeight;
                });
        }

        setInterval(() => {
            fetch("/status")
                .then(response => response.text())
                .then(data => {
                    if (data.trim()) {
                        document.getElementById("console").innerHTML += data + "<br>";
                        document.getElementById("console").scrollTop = document.getElementById("console").scrollHeight;
                    }
                });
        }, 1000);
    </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleWifi()
{
  server.send(200, "text/plain", "WiFi settings received");
}

// Обработчик команд
void handleCommand()
{
  String action = server.arg("action");
  String response = "OK";

  if (action == "blink_fast")
  {
    // Быстрое мигание на millis()
    unsigned long blinkStart = millis();
    int blinkCount = 0;
    bool ledState = HIGH;
    while (blinkCount < 10)
    {
      unsigned long now = millis();
      if ((now - blinkStart) >= 100)
      {
        blinkStart = now;
        digitalWrite(LED_PIN, ledState);
        ledState = !ledState;
        blinkCount++;
      }
      server.handleClient(); // чтобы не зависал сервер
      delay(1);
    }
    digitalWrite(LED_PIN, LOW);
    response = "Fast blink executed";
  }
  else if (action == "blink_slow")
  {
    // Медленное мигание на millis()
    unsigned long blinkStart = millis();
    int blinkCount = 0;
    bool ledState = HIGH;
    while (blinkCount < 10)
    {
      unsigned long now = millis();
      if ((now - blinkStart) >= 500)
      {
        blinkStart = now;
        digitalWrite(LED_PIN, ledState);
        ledState = !ledState;
        blinkCount++;
      }
      server.handleClient();
      delay(1);
    }
    digitalWrite(LED_PIN, LOW);
    response = "Slow blink executed";
  }
  else if (action == "led_on")
  {
    digitalWrite(LED_PIN, LOW);
    response = "LED ON";
  }
  else if (action == "led_off")
  {
    digitalWrite(LED_PIN, HIGH);
    response = "LED OFF";
  }
  else if (action == "restart")
  {
    response = "Restarting...";
    server.send(200, "text/plain", response);
    delay(1000);
    ESP.restart();
  }

  server.send(200, "text/plain", response);
}

// Статус устройства
void handleStatus()
{
  String status = "Time: " + String(millis()) + "ms | ";
  status += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes | ";
  status += "WiFi: ";
  status += String((WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected");
  server.send(200, "text/plain", status);
}

// OTA upload
void handleFileUpload()
{
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START)
  {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
    {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (Update.end(true))
    {
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  while (!Serial && millis() < 5000)
  {
    delay(10);
  }
  pinMode(LED_PIN, OUTPUT);

  prefs.begin("wifi", false);
  wifi_ssid = prefs.getString("ssid", "");
  wifi_pass = prefs.getString("pass", "");

  Serial.println();
  Serial.println("=== ESP32-C3 WiFi AP Starting ===");

  WiFi.mode(WIFI_AP_STA);

  // Start AP
  IPAddress ap_ip;
  String ap_ssid_with_ip;
  WiFi.softAP(ap_ssid_base, ap_password);
  ap_ip = WiFi.softAPIP();
  ap_ssid_with_ip = String(ap_ssid_base) + " (" + ap_ip.toString() + ")";
  WiFi.softAPsetHostname(ap_ssid_with_ip.c_str());

  // Try to connect to saved WiFi
  if (wifi_ssid.length() > 0)
  {
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    unsigned long startAttemptTime = millis();
    while (millis() - startAttemptTime < 5000)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        wifi_connected = true;
        break;
      }
      delay(100);
    }
  }

  server.on("/", handleRoot);
  server.on("/editor", handleCodeEditor);
  server.on("/wifi", HTTP_POST, handleWifi);
  server.on("/cmd", handleCommand);
  server.on("/status", handleStatus);
  server.on("/upload", HTTP_POST, []()
            {
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, handleFileUpload);
  server.begin();

  Serial.print("AP SSID: ");
  Serial.println(ap_ssid_with_ip);
  Serial.print("AP IP: ");
  Serial.println(ap_ip);
  Serial.println("=======================================");
}

void loop()
{
  server.handleClient();
}
