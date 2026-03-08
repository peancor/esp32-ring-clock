#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ctime>

#include "app_config.h"

class ChimeEffects {
public:
  enum class Frequency : uint8_t {
    NONE = 0,
    MINUTE,
    QUARTER,
    HOUR
  };

  enum class MinuteEffect : uint8_t {
    PIXEL_SPARK = 0,
    GENTLE_WAVE,
    COUNT
  };

  enum class QuarterEffect : uint8_t {
    DUAL_COMET = 0,
    GREEN_PULSE,
    COUNT
  };

  enum class HourEffect : uint8_t {
    RAINBOW_CHASE = 0,
    AURORA_WAVE,
    STAR_BURST,
    COUNT
  };

  struct Config {
    bool minuteEnabled = true;
    bool quarterEnabled = true;
    bool hourEnabled = true;

    bool minuteEffectEnabled[static_cast<uint8_t>(MinuteEffect::COUNT)] = { true, true };
    bool quarterEffectEnabled[static_cast<uint8_t>(QuarterEffect::COUNT)] = { true, true };
    bool hourEffectEnabled[static_cast<uint8_t>(HourEffect::COUNT)] = { true, true, true };

    uint16_t minuteDurationMs = 500;
    uint16_t quarterDurationMs = 1100;
    uint16_t hourDurationMs = 2200;
  };

  ChimeEffects() = default;

  void begin(uint32_t seed = 0);
  void update(const tm& localTime);
  void apply(Adafruit_NeoPixel& strip);

  Config& config();
  const Config& config() const;
  bool isActive() const;

private:
  Config cfg;

  int lastMinuteSeen = -1;
  Frequency activeFrequency = Frequency::NONE;
  uint8_t activeEffectId = 0;
  unsigned long activeStartMs = 0;
  uint16_t activeDurationMs = 0;

  static int wrap(int position);
  static uint32_t wheel(Adafruit_NeoPixel& strip, uint8_t pos);

  bool isExpired() const;
  void trigger(Frequency freq, uint8_t effectId, uint16_t durationMs);
  int pickRandomEnabled(const bool* enabledFlags, uint8_t count) const;

  void applyMinuteEffect(Adafruit_NeoPixel& strip, float progress);
  void applyQuarterEffect(Adafruit_NeoPixel& strip, float progress);
  void applyHourEffect(Adafruit_NeoPixel& strip, float progress);

  static void addToPixel(Adafruit_NeoPixel& strip, int index, uint8_t r, uint8_t g, uint8_t b);
};
