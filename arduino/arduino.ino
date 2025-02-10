#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>  // Include the Preferences library
#include <WiFiUdp.h>  // Include WiFi UDP library

// Define Access Point credentials (SSID and password)
const char* ap_ssid = "ESP32-AP";  // Name of your Access Point
const char* ap_password = "12345678";  // Password for the AP

WebServer server(80);  // Web server running on port 80

// ESP32 LED pin (you can connect an LED to this pin)
const int led_pin = 13;

const int neoPixelPin = 5;  // Pin to which NeoPixel is connected (use the appropriate GPIO)
const int numPixels = 1;  // Number of NeoPixels you have connected
Adafruit_NeoPixel strip(numPixels, neoPixelPin, NEO_GRB + NEO_KHZ800); // Setup NeoPixel

// Create a Preferences object to store data
Preferences preferences;

// Color variables
uint8_t winColor[3] = {0, 255, 0};  // Default to green for "Win"
uint8_t looseColor[3] = {255, 0, 0};  // Default to red for "Loose"

// UDP Setup
WiFiUDP udp;
const unsigned int udpPort = 12345;  // Port for receiving UDP messages

// Web Server: handle a request to / (root of the server)
void handleRoot() {
  String html = "<html><body><h1>ESP32 Control</h1>";
  html += "<form action='/setcolors' method='GET'>";
  html += "<label for='winColor'>Set Win Color (R,G,B): </label><br>";
  html += "<input type='text' id='winColor' name='winColor' value='" + String(winColor[0]) + "," + String(winColor[1]) + "," + String(winColor[2]) + "'><br>";
  html += "<label for='looseColor'>Set Loose Color (R,G,B): </label><br>";
  html += "<input type='text' id='looseColor' name='looseColor' value='" + String(looseColor[0]) + "," + String(looseColor[1]) + "," + String(looseColor[2]) + "'><br>";
  html += "<input type='submit' value='Set Colors'><br>";
  html += "</form>";
  html += "<button onclick=\"location.href='/win'\">Win</button><br>";
  html += "<button onclick=\"location.href='/loose'\">Loose</button><br>";
  html += "<button onclick=\"location.href='/off'\">Off</button><br>";  // Add Off button
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Function to handle "Win" button press
void handleWin() {
  strip.setPixelColor(0, strip.Color(winColor[0], winColor[1], winColor[2]));  // Set to the "Win" color
  strip.show();
  handleRoot();
}

// Function to handle "Loose" button press
void handleLoose() {
  strip.setPixelColor(0, strip.Color(looseColor[0], looseColor[1], looseColor[2]));  // Set to the "Loose" color
  strip.show();
  handleRoot();
}

void handleOff() {
  strip.setPixelColor(0, strip.Color(0, 0,0));  // Set to the "Loose" color
  strip.show();
  handleRoot();
}

// Function to handle setting the colors
void handleSetColors() {
  String winColorInput = server.arg("winColor");
  String looseColorInput = server.arg("looseColor");

  // Parse the input and set the colors
  int winR, winG, winB;
  int looseR, looseG, looseB;

  sscanf(winColorInput.c_str(), "%d,%d,%d", &winR, &winG, &winB);
  sscanf(looseColorInput.c_str(), "%d,%d,%d", &looseR, &looseG, &looseB);

  winColor[0] = winR;
  winColor[1] = winG;
  winColor[2] = winB;

  looseColor[0] = looseR;
  looseColor[1] = looseG;
  looseColor[2] = looseB;

  // Store the updated colors in the preferences (non-volatile storage)
  preferences.begin("colors", false);  // Open Preferences with namespace "colors"
  preferences.putInt("winR", winColor[0]);
  preferences.putInt("winG", winColor[1]);
  preferences.putInt("winB", winColor[2]);
  preferences.putInt("looseR", looseColor[0]);
  preferences.putInt("looseG", looseColor[1]);
  preferences.putInt("looseB", looseColor[2]);
  preferences.end();  // Close the Preferences

  server.send(200, "text/html", "<html><body><h1>Colors Updated!</h1><br><button onclick=\"location.href='/'\">Back</button></body></html>");
}

// Function to receive UDP message and control LED
void handleUDP() {
  int packetSize = udp.parsePacket();  // Check if we have any incoming UDP packet
  if (packetSize) {
    // Read the incoming UDP packet
    char packetBuffer[255];
    udp.read(packetBuffer, 255);
    
    // Process the received message (you can change the message contents as needed)
    String message = String(packetBuffer);
    if (message == "WIN") {
      strip.setPixelColor(0, strip.Color(winColor[0], winColor[1], winColor[2]));  // Set to Win color
      strip.show();
    } 
    else if (message == "LOOSE") {
      strip.setPixelColor(0, strip.Color(looseColor[0], looseColor[1], looseColor[2]));  // Set to Loose color
      strip.show();
    } 
    else if (message == "OFF") {
      strip.setPixelColor(0, strip.Color(0, 0, 0));  // Turn off the NeoPixel
      strip.show();
    }
  }
}

// Initializing everything at startup
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Setting up Access Point...");

  // Set up the LED pin as an output
  pinMode(led_pin, OUTPUT);

  // Start the ESP32 as an Access Point
  WiFi.softAP(ap_ssid, ap_password);  // Start AP with SSID and password
  
  // Log the IP address of the Access Point
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // mDNS (multicast DNS) setup for hostname resolution (optional)
  if (MDNS.begin("esp32")) {
    Serial.println("mDNS responder started");
  }

  // Initialize NeoPixel strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Load colors from preferences (if previously stored)
  preferences.begin("colors", true);  // Open Preferences in read mode
  winColor[0] = preferences.getInt("winR", 0);  // Default to 0 if not set
  winColor[1] = preferences.getInt("winG", 255);
  winColor[2] = preferences.getInt("winB", 0);
  
  looseColor[0] = preferences.getInt("looseR", 255);
  looseColor[1] = preferences.getInt("looseG", 0);
  looseColor[2] = preferences.getInt("looseB", 0);
  preferences.end();  // Close the Preferences

  // Web Server handlers:
  server.on("/", handleRoot);  // Handle root URL
  server.on("/win", handleWin);  // Handle win button
  server.on("/loose", handleLoose);  // Handle loose button
  server.on("/off", handleOff);  // Handle off button press
  server.on("/setcolors", HTTP_GET, handleSetColors);  // Handle color setting form
  server.onNotFound(handleNotFound);  // Handle other URLs

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");

  // Set up UDP listening
  udp.begin(udpPort);
  Serial.println("Listening for UDP packets on port " + String(udpPort));
}

void loop() {
  // Handle incoming client requests for web server
  server.handleClient();
  
  // Handle incoming UDP packets
  handleUDP();

  delay(2);  // Allow the CPU to switch to other tasks
}
