#pragma once

#include <Arduino.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#else
#error "Missing include/wifi_secrets.h. Copy include/wifi_secrets.example.h to include/wifi_secrets.h and fill in your WiFi credentials."
#endif

namespace AppConfig {

constexpr const char* SSID       = WifiSecrets::SSID;
constexpr const char* PASSWORD   = WifiSecrets::PASSWORD;
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr const char* TZ_INFO    = "CET-1CEST,M3.5.0,M10.5.0/3";

constexpr uint8_t LED_PIN     = 4;
constexpr uint8_t PIN_WS2812B = 16;
constexpr uint8_t NUM_LEDS    = 60;
constexpr uint8_t BUTTON_PIN  = 0;

constexpr int LED_OFFSET = 28;

constexpr uint8_t DAY_BRIGHTNESS   = 90;
constexpr uint8_t NIGHT_BRIGHTNESS = 15;
constexpr uint8_t NIGHT_START      = 22;
constexpr uint8_t NIGHT_END        = 7;

enum ClockMode : uint8_t {
  MODE_CLASSIC = 0,
  MODE_COMET,
  MODE_AMBIENT,
  MODE_NIGHT,
  MODE_SCIFI,
  MODE_COUNT
};

constexpr const char* MODE_NAMES[] = {
  "Classic", "Comet", "Ambient", "Night", "Sci-Fi"
};

} // namespace AppConfig
