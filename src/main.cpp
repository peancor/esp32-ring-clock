#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ctime>

#include "app_config.h"
#include "clock_renderer.h"
#include "chime_effects.h"

Adafruit_NeoPixel strip(AppConfig::NUM_LEDS, AppConfig::PIN_WS2812B, NEO_GRB + NEO_KHZ800);
ClockRenderer clockRenderer(strip);
ChimeEffects chimeEffects;

unsigned long lastButtonPress = 0;

void handleButton() {
  static bool lastState = HIGH;
  bool state = digitalRead(AppConfig::BUTTON_PIN);

  if (lastState == HIGH && state == LOW && millis() - lastButtonPress > 300) {
    lastButtonPress = millis();
    clockRenderer.nextMode();
    Serial.printf("► Modo: %s\n", clockRenderer.selectedModeName());

    for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(20, 20, 20));
    }
    strip.show();
    delay(80);
  }

  lastState = state;
}

void connectWifiWithAnimation() {
  WiFi.begin(AppConfig::SSID, AppConfig::PASSWORD);
  Serial.print("Conectando WiFi");

  int dot = 0;
  while (WiFi.status() != WL_CONNECTED) {
    strip.clear();
    strip.setPixelColor(dot % AppConfig::NUM_LEDS, strip.Color(0, 0, 40));
    strip.show();
    dot++;
    delay(100);
    Serial.print(".");
  }

  Serial.printf("\nWiFi OK · IP: %s\n", WiFi.localIP().toString().c_str());
}

void syncClockFromNtp() {
  configTime(0, 0, AppConfig::NTP_SERVER, "time.nist.gov");
  setenv("TZ", AppConfig::TZ_INFO, 1);
  tzset();

  Serial.print("Sincronizando NTP");
  while (time(nullptr) < 1000000000) {
    delay(500);
    Serial.print(".");
  }

  time_t now = time(nullptr);
  tm* local = localtime(&now);
  Serial.printf("\nHora: %02d:%02d:%02d\n", local->tm_hour, local->tm_min, local->tm_sec);
}

void setup() {
  Serial.begin(115200);
  pinMode(AppConfig::LED_PIN, OUTPUT);
  pinMode(AppConfig::BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(255);
  strip.show();

  connectWifiWithAnimation();
  syncClockFromNtp();

  clockRenderer.begin();
  chimeEffects.begin();

  ChimeEffects::Config& cfg = chimeEffects.config();
  cfg.minuteEnabled = true;
  cfg.quarterEnabled = true;
  cfg.hourEnabled = true;

  cfg.minuteEffectEnabled[static_cast<uint8_t>(ChimeEffects::MinuteEffect::PIXEL_SPARK)] = true;
  cfg.minuteEffectEnabled[static_cast<uint8_t>(ChimeEffects::MinuteEffect::GENTLE_WAVE)] = true;

  cfg.quarterEffectEnabled[static_cast<uint8_t>(ChimeEffects::QuarterEffect::DUAL_COMET)] = true;
  cfg.quarterEffectEnabled[static_cast<uint8_t>(ChimeEffects::QuarterEffect::GREEN_PULSE)] = true;

  cfg.hourEffectEnabled[static_cast<uint8_t>(ChimeEffects::HourEffect::RAINBOW_CHASE)] = true;
  cfg.hourEffectEnabled[static_cast<uint8_t>(ChimeEffects::HourEffect::AURORA_WAVE)] = true;
  cfg.hourEffectEnabled[static_cast<uint8_t>(ChimeEffects::HourEffect::STAR_BURST)] = true;
}

void loop() {
  handleButton();

  time_t now = time(nullptr);
  tm* local = localtime(&now);
  if (local == nullptr) {
    delay(16);
    return;
  }

  chimeEffects.update(*local);
  clockRenderer.render(*local);
  chimeEffects.apply(strip);

  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(AppConfig::LED_PIN, !digitalRead(AppConfig::LED_PIN));
  }

  delay(16);
}
