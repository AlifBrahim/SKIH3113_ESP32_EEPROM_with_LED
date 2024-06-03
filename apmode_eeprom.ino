#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>

// Constants
#define LED_PIN 13
#define EEPROM_SIZE 512

// Wi-Fi AP credentials for the configuration mode
const char *ap_ssid = "ESP32_Config_AP";
const char *ap_password = "password123";

// Web server
WebServer server(80);

// Variables for storing configuration
char ssid[32];
char password[32];
char deviceID[16];
bool ledStatus;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Read configuration from EEPROM
  if (!readConfig()) {
    // Start in AP mode if no valid configuration found
    startAPMode();
  } else {
    // Connect to Wi-Fi and restore LED status
    connectToWiFi();
    digitalWrite(LED_PIN, ledStatus ? HIGH : LOW);
  }
}

void loop() {
  // Handle web server
  server.handleClient();
}

bool readConfig() {
  EEPROM.get(0, ssid);
  EEPROM.get(32, password);
  EEPROM.get(64, deviceID);
  EEPROM.get(80, ledStatus);

  // Print read values for debugging
  Serial.println("Reading configuration from EEPROM...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
  Serial.print("LED Status: ");
  Serial.println(ledStatus ? "ON" : "OFF");

  // Validate SSID
  if (strlen(ssid) == 0) {
    return false;
  }
  return true;
}

void saveConfig() {
  Serial.println("Saving configuration to EEPROM...");
  EEPROM.put(0, ssid);
  EEPROM.put(32, password);
  EEPROM.put(64, deviceID);
  EEPROM.put(80, ledStatus);
  EEPROM.commit();
  
  // Print saved values for debugging
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
  Serial.print("LED Status: ");
  Serial.println(ledStatus ? "ON" : "OFF");
}

void startAPMode() {
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  String html = "<form action=\"/save\" method=\"POST\">";
  html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
  html += "Password: <input type=\"text\" name=\"password\" placeholder=\"Leave blank for open networks\"><br>";
  html += "Device ID: <input type=\"text\" name=\"deviceID\"><br>";
  html += "LED Status: <input type=\"radio\" name=\"ledStatus\" value=\"on\"> ON";
  html += "<input type=\"radio\" name=\"ledStatus\" value=\"off\"> OFF<br>";
  html += "<input type=\"submit\" value=\"Save\">";
  html += "</form>";
  server.send(200, "text/html", html);
}

void handleSave() {
  String ssidInput = server.arg("ssid");
  String passwordInput = server.arg("password");
  String deviceIDInput = server.arg("deviceID");
  String ledStatusStr = server.arg("ledStatus");

  if (ssidInput.length() > 0) {
    ssidInput.toCharArray(ssid, sizeof(ssid));
    passwordInput.toCharArray(password, sizeof(password));
    deviceIDInput.toCharArray(deviceID, sizeof(deviceID));
    ledStatus = (ledStatusStr == "on");

    saveConfig();
    server.send(200, "text/html", "Configuration saved. Rebooting...");
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "Invalid input");
  }
}

void connectToWiFi() {
  if (strlen(password) > 0) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }

  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect. Restarting in AP mode.");
    startAPMode();
  }
}
