#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ctime>

#include "app_config.h"

class ClockRenderer {
public:
  explicit ClockRenderer(Adafruit_NeoPixel& stripRef);

  void begin();
  void render(const tm& localTime);

  void nextMode();
  AppConfig::ClockMode selectedMode() const;
  const char* selectedModeName() const;

private:
  Adafruit_NeoPixel& strip;
  AppConfig::ClockMode currentMode = AppConfig::MODE_SCIFI;

  uint16_t bufR[AppConfig::NUM_LEDS]{};
  uint16_t bufG[AppConfig::NUM_LEDS]{};
  uint16_t bufB[AppConfig::NUM_LEDS]{};

  int prevSec = -1;
  int prevMin = -1;
  int prevHourPos = -1;

  int fadeOutSecPos = -1;
  int fadeOutMinPos = -1;
  int fadeOutHourPos = -1;

  unsigned long lastSecChange = 0;
  unsigned long lastMinChange = 0;
  unsigned long lastHourChange = 0;

  static int wrap(int position);
  static float breathe(unsigned long periodMs);
  static float expDecay(int distance, float rate);
  static float easeIn(unsigned long startMs, unsigned long durationMs);
  static float easeOut(unsigned long startMs, unsigned long durationMs);
  bool isNight(int h24) const;

  float subSecond() const;
  void clearBuf();
  void addPixel(int pos, uint16_t r, uint16_t g, uint16_t b);

  void layerSciFiGrid();
  void layerSecondProgress(int sec);
  void layerMinuteProgress(int minute);
  void layerHours(int hourPos, AppConfig::ClockMode mode);
  void layerHoursFade();
  void layerMinutes(int minPos, AppConfig::ClockMode mode, float easeF);
  void layerMinutesFade();
  void layerSeconds(int sec, AppConfig::ClockMode mode, float easeF);
  void layerSecondsFade();
  void layerTick(int sec, AppConfig::ClockMode mode);

  void fuseAndOutput(uint8_t brightness);
};
