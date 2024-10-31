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

// Define the soil, water sensor pins and pushButton
#define SOIL_SENSOR_PIN 33
#define WATER_SENSOR_PIN 34
#define BUTTON_PIN 32

// Server setup
WebServer server(80);

// LED pins
const int PUMP = 26;
const int LED2 = 27;

// RGB LED pins
const int RED_PIN = 23;
const int GREEN_PIN = 22;
const int BLUE_PIN = 21;

// LED and pump states
bool pumpState = false;
bool led2State = false;

// Global variables to store RGB values for HTML update
int redValue = 0, greenValue = 0, blueValue = 0;

// Function prototypes
void sendHtml();
void toggleBUTTON(int buttonNumber);
void updateSoilMoistureColor(int soilPercentage);
//void updateWaterLevelColor(int waterPercentage);

// Function to send HTML content to the web server
void sendHtml() {
  String response = R"(
    <!DOCTYPE html><html>
      <head>
        <title>GrowBox Web Server</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: sans-serif; text-align: center; }
          body { display: inline-flex; flex-direction: column; }
          h1 { margin-bottom: 1.2em; } 
          h2 { margin: 0; }
          
          div { 
            display: grid; 
            grid-template-columns: 1fr 1fr; 
            grid-template-rows: auto auto; 
            grid-auto-flow: column; 
            grid-gap: 1em; 
          }
          
          p.label { 
            margin-bottom: 0.1em; /* Reduced margin below labels to bring them closer to the bars. */
            font-weight: bold; 
          }

          html { font-family: sans-serif; text-align: center; }
          .bar.soil { background-color: #4CAF50; }
          .bar.temp { background-color: #F03333; }
          .bar.hum { background-color: #FFC107; }
          .bar.water { background-color: #2487DD; }
          .btn { background-color: #5B5; border: none; color: #fff; padding: 0.5em 1em;
                 font-size: 2em; text-decoration: none }
          .btn.OFF { background-color: #333; }
          
          .bar-container {
            width: 100%;
            background-color: #ddd;
            margin: 0.5em 0;          /* spacing around the container */
            border-radius: 10px;
            overflow: hidden;
            height: 30px;
            position: relative;
            //display: flex;            /* Text and bar to align vertically */
            //align-items: center;      /* Centers text vertically with the bar */
          }

          .bar {
            height: 100%;
            position: absolute;
            left: 0;
            top: 0;
            text-align: center;
            color: white;
            line-height: 30px;
            width: 0;
          }
          
          .bar-text {
            position: absolute;
            width: 100%;
            text-align: center;
            z-index: 1;
            line-height: 30px;
            color: black;
            font-weight: bold;
            //margin: 0;                /* Remove extra spacing */
          }

          .rgb-box {
            border: 2px solid #333;
            padding: 1em;
            margin: 1em 0;
          }
          
          .color-display {
            width: 100px;
            height: 100px;
            background-color: rgb(R_VAL, G_VAL, B_VAL);
            margin: 0 auto;
          }
        </style>
      </head>
            
      <body>
        <h1>GrowBox Web Server</h1>

        <div>
          <h2>Pump</h2>
          <a href="/toggle/1" class="btn PUMP_TEXT">PUMP_TEXT</a>
          <h2>LED 2</h2>
          <a href="/toggle/2" class="btn LED2_TEXT">LED2_TEXT</a>
        </div>

        <div>
          <p class="label">Temperature: TEMP &deg;C</p>
          <div class="bar-container">
            <div class="bar temp" style="width: TEMP%;"></div>
            <div class="bar-text">TEMP &deg;C</div>
          </div>
          
          <p class="label">Humidity: HUM %</p>
          <div class="bar-container">
            <div class="bar hum" style="width: HUM%;"></div>
            <div class="bar-text">HUM %</div>
          </div>
        </div>
        
        <div>
          <p class="label">Soil Moisture: SOIL %</p>
          <div class="bar-container">
            <div class="bar soil" style="width: SOIL%;"></div>
            <div class="bar-text">SOIL %</div>
          </div>

          <p class="label">Water level: WATER %</p>
          <div class="bar-container">
            <div class="bar water" style="width: WATER%;"></div>
            <div class="bar-text">WATER %</div>
          </div>
        </div>

        <div class="rgb-box">
          <h2>RGB LED</h2>
          <div class="color-display"></div>
          <p>Red: R_VAL</p>
          <p>Green: G_VAL</p>
          <p>Blue: B_VAL</p>
        </div>

      </body>
    </html>
  )";

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilMoisture = analogRead(SOIL_SENSOR_PIN);
  int soilPercentage = map(soilMoisture, 0, 4095, 0, 100);
  int waterLevel = analogRead(WATER_SENSOR_PIN);
  int waterPercentage = map(waterLevel, 0, 4095, 0, 100);

  if (soilPercentage > 80) {
    pumpState = false;               // Set PUMP state to OFF
    digitalWrite(PUMP, pumpState);   // Stop the pump
  }

  if (isnan(temperature) || isnan(humidity)) {
    response.replace("TEMP", "0");
    response.replace("HUM", "0");
  } else {
    int tempPercentage = constrain(temperature, 0, 100);
    int humPercentage = constrain(humidity, 0, 100);
    response.replace("TEMP", String(tempPercentage));
    response.replace("HUM", String(humPercentage));
    response.replace("SOIL", String(soilPercentage));
    response.replace("WATER", String(waterPercentage));

    // Update RGB color based on soil percentage and store values
    updateSoilMoistureColor(soilPercentage);
  }

  // Update LED states in the HTML response
  response.replace("PUMP_TEXT", pumpState ? "ON" : "OFF");
  response.replace("LED2_TEXT", led2State ? "ON" : "OFF");

  // Replace RGB values in HTML
  response.replace("R_VAL", String(redValue));
  response.replace("G_VAL", String(greenValue));
  response.replace("B_VAL", String(blueValue));

  server.send(200, "text/html", response);
}

// Set RGB color based on soil moisture percentage
void updateSoilMoistureColor(int soilPercentage) {
  if (soilPercentage < 20) {         // Dry: Red
    redValue = 255;
    greenValue = 0;
    blueValue = 0;
  } else if (soilPercentage <= 60) { // Moist: Orange
    redValue = 255;
    greenValue = 165; 
    blueValue = 0;
  } else {                           // Wet: Green
    redValue = 0;
    greenValue = 255;
    blueValue = 0;
  }

  // Update RGB LED with PWM values
  analogWrite(RED_PIN, redValue);
  analogWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, blueValue);
}

// Toggle LED based on the request
void toggleBUTTON(int buttonNumber) {
  switch (buttonNumber) {
    case 1:
      pumpState = !pumpState;
      digitalWrite(PUMP, pumpState);
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

  // Initialize pushButton as input with pull-up
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP, OUTPUT);
  
  // Initialize LED pins as outputs
  pinMode(PUMP, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  // Initialize RGB pins as outputs
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

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

  // Handle BUTTON toggling via web interface
  server.on(UriBraces("/toggle/{}"), []() {
    String led = server.pathArg(0);
    Serial.print("Toggle PUMP");
    Serial.println(led);

    toggleBUTTON(led.toInt());

    // After toggling the LED, redirect back to the main page
    server.sendHeader("Location", "/");  // Redirect to home page
    server.send(303);  // 303 See Other, browser will follow the redirect
  });

  // Start the web server
  server.begin();
  Serial.println("HTTP server started (http://localhost:8180)");
  
  // Add this line in setup() after defining other routes
  server.onNotFound([]() {
    Serial.print("Unhandled request for: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "404: Not Found");
  });

  server.on("/favicon.ico", []() {
    server.send(204);  // 204 No Content: a minimal response to satisfy the request
});
}

// Main loop function.
void loop(void) {

  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW) {  // LOW means pressed
    pumpState = !pumpState;  // Toggle PUMP state
    digitalWrite(PUMP, pumpState);
    delay(200);  // Debounce delay
  }

  // Handle client requests
  server.handleClient();
  delay(2);
}
