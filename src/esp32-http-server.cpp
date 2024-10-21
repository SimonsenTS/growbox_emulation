#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include "DHT.h"

// WiFi configuration
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// DHT sensor configuration
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Server setup
WebServer server(80);

// LED pins
const int LED1 = 26;
const int LED2 = 27;

// LED states
bool led1State = false;
bool led2State = false;

// Function prototypes
void sendHtml();
void toggleLED(int ledNumber);

// Function to send HTML content to the web server
void sendHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>ESP32 Web Server Demo</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          div { display: grid; grid-template-columns: 1fr 1fr; grid-template-rows: auto auto; grid-auto-flow: column; grid-gap: 1em; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
        </style>
      </head>
            
      <body>
        <h1>ESP32 Web Server</h1>

        <div>
          <h2>LED 1</h2>
          <a href="/toggle/1" class="btn LED1_TEXT">LED1_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>
        <p>Temperature: TEMP °C</p>
        <p>Humidity: HUM %</p>
      </body>
    </html>
  )";

  // Reading temperature and humidity from the DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    response.replace("TEMP", "N/A");
    response.replace("HUM", "N/A");
  } else {
    response.replace("TEMP", String(temperature));
    response.replace("HUM", String(humidity));
  }

  // Update LED states in the HTML response
  response.replace("LED1_TEXT", led1State ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");
  
  // Send the HTML response to the client
  server.send(200, "text/html", response);
}

// Toggle LED based on the request
void toggleLED(int ledNumber) {
  switch (ledNumber) {
    case 1:
      led1State = !led1State;
      digitalWrite(LED1, led1State);
      break;
    case 2:
      led2State = !led2State;
      digitalWrite(LED2, led2State);
      break;
  }
}

// Setup function for initializing the ESP32
void setup(void) {
  Serial.begin(115200);

  // Initialize the DHT sensor
  dht.begin();
  
  // Initialize LED pins as outputs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Print IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up server routes
  server.on("/", sendHtml);

  // Handle LED toggling via web interface
  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle LED #");
    Serial.println(led);

    toggleLED(led.toInt());

    // After toggling the LED, redirect back to the main page
    server.sendHeader("Location", "/");  // Redirect to home page
    server.send(303);  // 303 See Other, browser will follow the redirect
  });

  // Start the web server
  server.begin();
  Serial.println("HTTP server started (http://localhost:8180)");
}

// Main loop function
void loop(void) {
  // Handle client requests
  server.handleClient();
  delay(2);
}
