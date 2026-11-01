# PixelWeather

**A compact ESP32-powered weather station with a colour TFT display and live weather data from Open-Meteo.**

PixelWeather is an embedded weather dashboard built on an ESP32  board and a TFT display. It retrieves live weather information from the Open-Meteo API, and displays current conditions and daily forecasts

## Features

- **Live weather data:** Retrieves current weather conditions using the Open-Meteo Forecast API.
- **Three display screens:** Switch between current weather, atmospheric conditions, and daily forecasts.
- **Automatic updates:** Refreshes weather data every 15 minutes.
- **Manual refresh:** Update weather information by pressing the refresh button

## Display Interface

The interface  contains three screens:

**01 — Current Weather**
- Current temperature
- Feels-like temperature
- Weather description

**02 — Conditions**
- Relative humidity
- Wind speed

**03 — Daily Forecast**
- Daily maximum temperature
- Daily minimum temperature

### Button Controls

| Button | Function |
|---|---|
| Refresh | Manually retrieve updated weather data |
| Next Screen | Cycle through the three display screens |

Weather data is automatically refreshed every 15 minutes. Failed updates trigger a shorter retry interval.

<img width="683" height="591" alt="image" src="https://github.com/user-attachments/assets/14611abb-4cd1-41b3-a15e-c36d4db204c5" />
<img width="413" height="551" alt="image" src="https://github.com/user-attachments/assets/24dcf338-e0f8-493c-89ff-7644cbdbb07f" />
<img width="1662" height="746" alt="image" src="https://github.com/user-attachments/assets/2bba03b5-f6ec-4715-aa4b-19ebb7db4e7e" />
