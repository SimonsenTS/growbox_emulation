#include "WebPage.h"
#include "Config.h"

String WebPage::getHTML() {
    return R"delimiter(
  <!DOCTYPE html>
    <html>
      <head>
        <div id="time-box" class="info-box">
          <p><strong>Time & Date:</strong> <span id="current-time"></span> | <span id="current-date"></span></p>
        </div>

        <title>GrowBox Web Server</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html {
            font-family: sans-serif;
            text-align: center;
          }

          body {
            display: flex;
            flex-direction: column;
            align-items: center;
          }

          h1 {
            margin-bottom: 1.2em;
          }

          h2 {
            margin: 0;
          }

          div {
            display: grid;
            grid-template-columns: 1fr 1fr;
            grid-template-rows: auto auto;
            grid-auto-flow: column;
            grid-gap: 1em;
          }

          p.label {
            margin-bottom: 0.1em;
            font-weight: bold;
          }

          html {
            font-family: sans-serif;
            text-align: center;
          }

          .container {
            width: 100%;
            max-width: 450px;
            margin: 0 auto;
            text-align: center;
          }

          .bar.soil {
            background-color: #4caf50;
          }
          .bar.temp {
            background-color: #f5532f;
          }
          .bar.hum {
            background-color: #ffc107;
          }
          .bar.water {
            background-color: #2487dd;
          }

          .btn {
            padding: 1em;
            font-size: 1.2em;
            margin: 0.5em 0;
            background-color: #5b5;
            border: none;
            color: #fff;
            text-decoration: none;
            border-radius: 10px;
          }
          
          .btn-off {
            padding: 1em;
            font-size: 1.2em;
            margin: 0.5em 0;
            background-color: #333;
            border: none;
            color: #fff;
            text-decoration: none;
            border-radius: 10px;
          }

          .bar-container {
            width: 100%;
            background-color: #ddd;
            margin: 0.5em 0;
            border-radius: 10px;
            overflow: hidden;
            height: 30px;
            position: relative;
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
            margin: 0;
          }

          .rgb-container {
            display: flex;
            justify-content: center;
            gap: 20px;
            flex-wrap: wrap;
            margin: 20px auto;
            max-width: 600px;
          }

          .rgb-box {
            border: 2px solid #333;
            padding: 1em;
            flex: 1;
            min-width: 200px;
            max-width: 280px;
            border-radius: 10px;
            background: #f9f9f9;
          }

          .rgb-box h3 {
            margin-top: 0;
            color: #333;
            font-size: 1.1em;
          }

          .color-display {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            margin: 10px auto;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.2);
            transition: background-color 0.3s ease;
          }

          .slider-container {
            width: 100%;
            max-width: 450px;
            margin: 0 auto;
          }
            
          .slider-container input[type="range"] {
            appearance: none;
            width: 205%;
            height: 10px;
            background: #ddd;
            border-radius: 5px;
            outline: none;
            opacity: 0.8;
            transition: opacity 0.2s;
            display: block;
            margin: 0 auto;
          }
            
          .slider-container input[type="range"]:hover {
            opacity: 1;
          }

          .slider-label {
            font-weight: bold;
            margin-bottom: 0.5em;
          }

          .info-box {
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 0.5em;
            border: 1px solid #ddd;
            border-radius: 5px;
            background-color: #f9f9f9;
            width: 100%;
            max-width: 400px;
            margin: 1em auto;
            text-align: center;
            font-size: 1em;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
          }

          .info-box p {
            margin: 0;
            font-size: 1em;
            line-height: 1.5;
          }

)delimiter"
#if SIMULATION_MODE
R"delimiter(
          /* Simulation Panel */
          .sim-panel {
            background: #fff3cd;
            border: 2px solid #ffc107;
            border-radius: 10px;
            padding: 20px;
            margin: 20px auto;
            max-width: 450px;
          }

          .sim-title {
            color: #856404;
            margin-bottom: 15px;
            font-size: 1.3em;
          }

          .sim-control {
            margin: 10px 0;
            text-align: left;
          }

          .sim-control label {
            display: inline-block;
            width: 120px;
            font-weight: bold;
            color: #856404;
          }

          .sim-control input[type="number"] {
            width: 80px;
            padding: 5px;
            border: 1px solid #ffc107;
            border-radius: 5px;
            margin-right: 10px;
          }

          .sim-btn {
            background-color: #ffc107;
            color: #856404;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            font-weight: bold;
            margin-top: 10px;
            width: 100%;
          }

          .sim-btn:hover {
            background-color: #e0a800;
          }

)delimiter"
#endif
R"delimiter(
        </style>
      </head>

      <body>
        <h1>GrowBox Web Server</h1>

)delimiter"
#if SIMULATION_MODE
R"delimiter(
        <!-- Simulation Panel -->
        <div class="sim-panel container">
          <h2 class="sim-title">Simulation Controls</h2>
          <div class="sim-control">
            <label>Temperature:</label>
            <input type="number" id="simTemp" min="-40" max="80" step="0.5" value="TEMP">
            <span>&deg;C</span>
          </div>
          <div class="sim-control">
            <label>Humidity:</label>
            <input type="number" id="simHum" min="0" max="100" step="1" value="HUM">
            <span>%</span>
          </div>
          <div class="sim-control">
            <label>Soil Moisture:</label>
            <input type="number" id="simSoil" min="0" max="100" step="1" value="SOIL">
            <span>%</span>
          </div>
          <div class="sim-control">
            <label>Water Level:</label>
            <input type="number" id="simWater" min="0" max="100" step="1" value="WATER">
            <span>%</span>
          </div>
          <button class="sim-btn" onclick="updateSimulation()">Update Simulation</button>
        </div>

)delimiter"
#endif
R"delimiter(
        <!-- Buttons -->
        <div class="container">
          <h2>Pump</h2>
          <a href="/toggle/1" class="PUMP_CLASS">PUMP_TEXT</a>
          <h2>Grow LED</h2>
          <a href="/toggle/2" class="LED_CLASS">GrowLED</a>
          <h2>LED Boost</h2>
          <a href="/toggle/4" class="BOOST_CLASS">BOOST_TEXT</a>
          <h2>Status LEDs</h2>
          <a href="/toggle/3" class="RGB_CLASS">RGB_LED</a>
        </div>

        <!-- Bars -->
        <div class="container">
          <p class="label">Temperature: TEMP &deg;C</p>
          <div class="bar-container">
            <div class="bar temp" style="width: TEMP%"></div>
            <div class="bar-text">TEMP &deg;C</div>
          </div>

          <p class="label">Humidity: HUM %</p>
          <div class="bar-container">
            <div class="bar hum" style="width: HUM%"></div>
            <div class="bar-text">HUM %</div>
          </div>
        </div>

        <div class="container">
          <p class="label">Soil Moisture: SOIL %</p>
          <div class="bar-container">
            <div class="bar soil" style="width: SOIL%"></div>
            <div class="bar-text">SOIL %</div>
          </div>

          <p class="label">Water Level: WATER %</p>
          <div class="bar-container">
            <div class="bar water" style="width: WATER%"></div>
            <div class="bar-text">WATER %</div>
          </div>
        </div>

        <!-- Slider -->
        <div class="slider-container">
          <p class="slider-label">
            Brightness: <span id="brightnessValue">BRIGHTNESS%</span>
          </p>
          <input
            type="range"
            min="0"
            max="100"
            value="BRIGHTNESS"
            onchange=updateBrightness(this.value)
          />
        </div>

        <!-- RGB LEDs Container -->
        <div class="rgb-container">
          <div class="rgb-box">
            <h3>Soil Moisture LED</h3>
            <div id="soilLED" class="color-display" style="background-color: rgb(SOIL_R_VAL, SOIL_G_VAL, SOIL_B_VAL);"></div>
          </div>
          
          <div class="rgb-box">
            <h3>Water Level LED</h3>
            <div id="waterLED" class="color-display" style="background-color: rgb(WATER_R_VAL, WATER_G_VAL, WATER_B_VAL);"></div>
          </div>
        </div>

        <script>
          function updateBrightness(value) {
            document.getElementById("brightnessValue").innerText = value + "%";
            fetch("/brightness/" + value);
          }

)delimiter"
#if SIMULATION_MODE
R"delimiter(
          function updateSimulation() {
            const temp = document.getElementById("simTemp").value;
            const hum = document.getElementById("simHum").value;
            const soil = document.getElementById("simSoil").value;
            const water = document.getElementById("simWater").value;

            const formData = new URLSearchParams();
            formData.append("temperature", temp);
            formData.append("humidity", hum);
            formData.append("soil", soil);
            formData.append("water", water);

            fetch("/simulation", {
              method: "POST",
              headers: {
                "Content-Type": "application/x-www-form-urlencoded",
              },
              body: formData
            })
            .then(response => response.text())
            .then(data => {
              console.log("Simulation updated:", data);
              setTimeout(() => window.location.reload(), 500);
            })
            .catch(error => console.error("Error:", error));
          }

)delimiter"
#endif
R"delimiter(
          function updateTime() {
            const now = new Date();
            const time = now.toLocaleTimeString();
            const date = now.toLocaleDateString();

            document.getElementById("current-time").innerText = time;
            document.getElementById("current-date").innerText = date;
          }

          setInterval(updateTime, 1000);
          updateTime();

        </script>
      </body>
    </html>
  )delimiter";
}
