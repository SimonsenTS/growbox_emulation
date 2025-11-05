#include "WebPage.h"

String WebPage::getHTML() {
    return R"(
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
          
          .btn.OFF {
            background-color: #333;
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

          .rgb-box {
            border: 2px solid #333;
            padding: 1em;
            margin: 1em 0;
            max-width: 380px;
            width: 100%;
          }

          .color-display {
            width: 100px;
            height: 100px;
            background-color: rgb(R_VAL, G_VAL, B_VAL);
            margin: 0 auto;
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

        </style>
      </head>

      <body>
        <h1>GrowBox Web Server</h1>

        <!-- Buttons -->
        <div class="container">
          <h2>Pump</h2>
          <a href="/toggle/1" class="btn PUMP_TEXT">PUMP_TEXT</a>
          <h2>Grow LED</h2>
          <a href="/toggle/2" class="btn GrowLED">GrowLED</a>
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

        <!-- RGB Box -->
        <div class="rgb-box container">
          <h2>RGB LED</h2>
          <div class="color-display"></div>
          <p>Red: R_VAL</p>
          <p>Green: G_VAL</p>
          <p>Blue: B_VAL</p>
        </div>

        <script>
          function updateBrightness(value) {
            document.getElementById("brightnessValue").innerText = value + "%";
            fetch('/brightness/' + value);
          }

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
  )";
}
