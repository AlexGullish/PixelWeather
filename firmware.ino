
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// TFT
#define TFT_CS    21
#define TFT_DC    22
#define TFT_RST   16
#define TFT_BL    25
#define TFT_MOSI  23
#define TFT_SCLK  18

// Buttons
#define BTN_REFRESH 32
#define BTN_NEXT    33


const char* WIFI_SSID = "YOUR_WIFI_NAME"; //I'll put wifi here later 
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";


const float LATITUDE = 46.2044;
const float LONGITUDE = 6.1437;

const unsigned long UPDATE_INTERVAL = 15UL * 60UL * 1000UL;


Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

const uint16_t BG       = 0x0861;
const uint16_t PANEL    = 0x10A3;
const uint16_t WHITE    = 0xFFFF;
const uint16_t MUTED    = 0x8410;
const uint16_t CYAN     = 0x4DFF;
const uint16_t YELLOW   = 0xFFE0;
const uint16_t LINE     = 0x2945;


struct WeatherData {
  float temperature = 0;
  float feelsLike = 0;
  float humidity = 0;
  float windSpeed = 0;

  float high = 0;
  float low = 0;

  int weatherCode = 0;
  int rainChance = 0;

  bool valid = false;
};

WeatherData weather;

int currentScreen = 0;
const int SCREEN_COUNT = 3;

unsigned long nextWeatherUpdate = 0;


struct Button {
  uint8_t pin;
  bool lastReading;
  bool stableState;
  unsigned long lastChange;
};

Button refreshButton = {BTN_REFRESH, HIGH, HIGH, 0};
Button nextButton = {BTN_NEXT, HIGH, HIGH, 0};

bool buttonPressed(Button &button) {
  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {
    button.lastChange = millis();
    button.lastReading = reading;
  }

  if (millis() - button.lastChange > 35 &&
      reading != button.stableState) {

    button.stableState = reading;

    if (button.stableState == LOW) {
      return true;
    }
  }

  return false;
}


const char* weatherDescription(int code) {
  if (code == 0) return "Clear sky";
  if (code <= 2) return "Partly cloudy";
  if (code == 3) return "Overcast";

  if (code == 45 || code == 48)
    return "Foggy";

  if (code >= 51 && code <= 57)
    return "Drizzle";

  if (code >= 61 && code <= 67)
    return "Rain";

  if (code >= 71 && code <= 77)
    return "Snow";

  if (code >= 80 && code <= 82)
    return "Rain showers";

  if (code == 85 || code == 86)
    return "Snow showers";

  if (code >= 95)
    return "Thunderstorm";

  return "Unknown";
}


void drawCentered(
  const String &message,
  int y,
  uint8_t textSize,
  uint16_t color
) {
  tft.setTextSize(textSize);
  tft.setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;

  tft.getTextBounds(
    message.c_str(),
    0, y,
    &x1, &y1,
    &w, &h
  );

  int x = (tft.width() - w) / 2;
  tft.setCursor(x, y);
  tft.print(message);
}

void drawHeader(const char* title) {
  tft.fillScreen(BG);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(7, 7);
  tft.print("PIXELWEATHER");

  tft.setCursor(106, 7);
  tft.print(title);

  tft.drawFastHLine(5, 22, 150, LINE);
}

void drawFooter() {
  tft.drawFastHLine(5, 111, 150, LINE);

  tft.setTextSize(1);
  tft.setTextColor(MUTED);

  tft.setCursor(7, 116);
  tft.print("NEXT: SCREEN");

  tft.setCursor(102, 116);
  tft.print("OPEN-METEO");
}



void drawCurrentScreen() {
  drawHeader("01");

  if (!weather.valid) {
    drawCentered("Waiting for data", 48, 1, WHITE);
    drawCentered("Check WiFi", 68, 1, CYAN);
    drawFooter();
    return;
  }

  drawCentered(
    weatherDescription(weather.weatherCode),
    31, 1, CYAN
  );

  String temperature =
    String((int)round(weather.temperature)) + " C";

  drawCentered(
    temperature,
    49, 4, WHITE
  );

  String feelsLike =
    "Feels like " +
    String((int)round(weather.feelsLike)) + " C";

  drawCentered(
    feelsLike,
    94, 1, MUTED
  );

  drawFooter();
}



void drawConditionsScreen() {
  drawHeader("02");

  if (!weather.valid) {
    drawCentered("No weather data", 55, 1, WHITE);
    drawFooter();
    return;
  }

  tft.setTextSize(1);
  tft.setTextColor(MUTED);

  tft.setCursor(15, 36);
  tft.print("HUMIDITY");

  tft.setCursor(15, 76);
  tft.print("WIND SPEED");

  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(15, 49);
  tft.print(String((int)round(weather.humidity)) + "%");

  tft.setCursor(15, 89);
  tft.print(String((int)round(weather.windSpeed)) + " km/h");

  drawFooter();
}


void drawForecastScreen() {
  drawHeader("03");

  if (!weather.valid) {
    drawCentered("No weather data", 55, 1, WHITE);
    drawFooter();
    return;
  }

  tft.setTextSize(1);
  tft.setTextColor(MUTED);

  tft.setCursor(15, 35);
  tft.print("TODAY'S HIGH");

  tft.setCursor(15, 71);
  tft.print("TODAY'S LOW");

  tft.setTextSize(2);
  tft.setTextColor(YELLOW);

  tft.setCursor(15, 48);
  tft.print(String((int)round(weather.high)) + " C");

  tft.setTextColor(CYAN);

  tft.setCursor(15, 84);
  tft.print(String((int)round(weather.low)) + " C");

  drawFooter();
}


void drawScreen() {
  switch (currentScreen) {
    case 0:
      drawCurrentScreen();
      break;

    case 1:
      drawConditionsScreen();
      break;

    case 2:
      drawForecastScreen();
      break;
  }
}


bool connectWiFi() {
  if (String(WIFI_SSID) == "YOUR_WIFI_NAME") {
    Serial.println("Set your WiFi credentials first.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {

    Serial.print(".");
    delay(300);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi connection failed.");
  return false;
}


bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected.");
    return false;
  }

  String url =
    "https://api.open-meteo.com/v1/forecast?"
    "latitude=" + String(LATITUDE, 4) +
    "&longitude=" + String(LONGITUDE, 4) +
    "&current=temperature_2m,"
    "relative_humidity_2m,"
    "apparent_temperature,"
    "weather_code,"
    "wind_speed_10m"
    "&daily=temperature_2m_max,"
    "temperature_2m_min,"
    "precipitation_probability_max"
    "&forecast_days=1"
    "&timezone=auto";

  WiFiClientSecure client;


  client.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);

  if (!http.begin(client, url)) {
    Serial.println("HTTP connection failed.");
    return false;
  }

  int responseCode = http.GET();

  if (responseCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", responseCode);
    http.end();
    return false;
  }

  JsonDocument doc;

  DeserializationError error =
    deserializeJson(doc, http.getStream());

  http.end();

  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject current = doc["current"];
  JsonObject daily = doc["daily"];

  if (current.isNull() || daily.isNull()) {
    Serial.println("Weather response missing data.");
    return false;
  }

  weather.temperature =
    current["temperature_2m"] | 0.0f;

  weather.feelsLike =
    current["apparent_temperature"] | 0.0f;

  weather.humidity =
    current["relative_humidity_2m"] | 0.0f;

  weather.windSpeed =
    current["wind_speed_10m"] | 0.0f;

  weather.weatherCode =
    current["weather_code"] | 0;

  weather.high =
    daily["temperature_2m_max"][0] | 0.0f;

  weather.low =
    daily["temperature_2m_min"][0] | 0.0f;

  weather.rainChance =
    daily["precipitation_probability_max"][0] | 0;

  weather.valid = true;

  Serial.println("Weather updated successfully.");

  Serial.printf(
    "Temperature: %.1f C\n",
    weather.temperature
  );

  Serial.printf(
    "Humidity: %.0f%%\n",
    weather.humidity
  );

  Serial.printf(
    "Wind: %.1f km/h\n",
    weather.windSpeed
  );

  return true;
}

void setup() {
  Serial.begin(115200);

  // Buttons
  pinMode(BTN_REFRESH, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  // SPI configuration
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // TFT initialization
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.initR(INITR_GREENTAB);
  tft.setRotation(1);
  tft.setTextWrap(false);

  tft.fillScreen(BG);

  drawCentered("PIXELWEATHER", 35, 2, CYAN);
  drawCentered("Starting up...", 70, 1, WHITE);

  // Connect to WiFi
  if (connectWiFi()) {
    fetchWeather();
  }

  drawScreen();

  nextWeatherUpdate =
    millis() + UPDATE_INTERVAL;
}


void loop() {


  if (buttonPressed(nextButton)) {
    currentScreen =
      (currentScreen + 1) % SCREEN_COUNT;

    drawScreen();
  }


  if (buttonPressed(refreshButton)) {
    tft.fillScreen(BG);
    drawCentered("Refreshing...", 55, 1, CYAN);

    bool success = fetchWeather();

    drawScreen();

    nextWeatherUpdate = millis() +
      (success ? UPDATE_INTERVAL : 60000UL);
  }


  if ((long)(millis() - nextWeatherUpdate) >= 0) {

    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }

    bool success = fetchWeather();

    nextWeatherUpdate = millis() +
      (success ? UPDATE_INTERVAL : 60000UL);

    drawScreen();
  }

  delay(10);
}
