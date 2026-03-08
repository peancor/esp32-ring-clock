#include "chime_effects.h"

void ChimeEffects::begin(uint32_t seed) {
  randomSeed(seed == 0 ? micros() : seed);
}

ChimeEffects::Config& ChimeEffects::config() {
  return cfg;
}

const ChimeEffects::Config& ChimeEffects::config() const {
  return cfg;
}

int ChimeEffects::wrap(int position) {
  return (((position + AppConfig::LED_OFFSET) % AppConfig::NUM_LEDS) + AppConfig::NUM_LEDS) % AppConfig::NUM_LEDS;
}

bool ChimeEffects::isExpired() const {
  if (activeFrequency == Frequency::NONE) return true;
  return (millis() - activeStartMs) > activeDurationMs;
}

bool ChimeEffects::isActive() const {
  return activeFrequency != Frequency::NONE && !isExpired();
}

void ChimeEffects::trigger(Frequency freq, uint8_t effectId, uint16_t durationMs) {
  activeFrequency = freq;
  activeEffectId = effectId;
  activeDurationMs = durationMs;
  activeStartMs = millis();
}

int ChimeEffects::pickRandomEnabled(const bool* enabledFlags, uint8_t count) const {
  uint8_t enabledIndexes[8]{};
  uint8_t enabledCount = 0;

  for (uint8_t i = 0; i < count; i++) {
    if (enabledFlags[i]) {
      enabledIndexes[enabledCount++] = i;
    }
  }

  if (enabledCount == 0) return -1;
  return enabledIndexes[random(enabledCount)];
}

void ChimeEffects::update(const tm& localTime) {
  if (isExpired()) {
    activeFrequency = Frequency::NONE;
  }

  if (localTime.tm_min == lastMinuteSeen) return;
  lastMinuteSeen = localTime.tm_min;

  if (localTime.tm_min == 0 && cfg.hourEnabled) {
    int chosen = pickRandomEnabled(cfg.hourEffectEnabled, static_cast<uint8_t>(HourEffect::COUNT));
    if (chosen >= 0) {
      trigger(Frequency::HOUR, static_cast<uint8_t>(chosen), cfg.hourDurationMs);
      return;
    }
  }

  if ((localTime.tm_min % 15) == 0 && cfg.quarterEnabled) {
    int chosen = pickRandomEnabled(cfg.quarterEffectEnabled, static_cast<uint8_t>(QuarterEffect::COUNT));
    if (chosen >= 0) {
      trigger(Frequency::QUARTER, static_cast<uint8_t>(chosen), cfg.quarterDurationMs);
      return;
    }
  }

  if (cfg.minuteEnabled) {
    int chosen = pickRandomEnabled(cfg.minuteEffectEnabled, static_cast<uint8_t>(MinuteEffect::COUNT));
    if (chosen >= 0) {
      trigger(Frequency::MINUTE, static_cast<uint8_t>(chosen), cfg.minuteDurationMs);
    }
  }
}

void ChimeEffects::addToPixel(Adafruit_NeoPixel& strip, int index, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t base = strip.getPixelColor(index);
  uint8_t br = static_cast<uint8_t>((base >> 16) & 0xFF);
  uint8_t bg = static_cast<uint8_t>((base >> 8) & 0xFF);
  uint8_t bb = static_cast<uint8_t>(base & 0xFF);

  uint16_t rr = br + r;
  uint16_t gg = bg + g;
  uint16_t bb2 = bb + b;

  if (rr > 255) rr = 255;
  if (gg > 255) gg = 255;
  if (bb2 > 255) bb2 = 255;

  strip.setPixelColor(index, rr, gg, bb2);
}

uint32_t ChimeEffects::wheel(Adafruit_NeoPixel& strip, uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}

void ChimeEffects::applyMinuteEffect(Adafruit_NeoPixel& strip, float progress) {
  auto effect = static_cast<MinuteEffect>(activeEffectId);
  switch (effect) {
    case MinuteEffect::PIXEL_SPARK: {
      int center = static_cast<int>(progress * AppConfig::NUM_LEDS) % AppConfig::NUM_LEDS;
      addToPixel(strip, wrap(center), 0, 15, 35);
      addToPixel(strip, wrap(center - 1), 0, 8, 18);
      addToPixel(strip, wrap(center + 1), 0, 8, 18);
      break;
    }
    case MinuteEffect::GENTLE_WAVE: {
      float wave = sinf(progress * 2.0f * PI);
      uint8_t blue = static_cast<uint8_t>(12 + (wave + 1.0f) * 10.0f);
      for (int i = 0; i < AppConfig::NUM_LEDS; i += 5) {
        addToPixel(strip, wrap(i), 0, 0, blue);
      }
      break;
    }
    default:
      break;
  }
}

void ChimeEffects::applyQuarterEffect(Adafruit_NeoPixel& strip, float progress) {
  auto effect = static_cast<QuarterEffect>(activeEffectId);
  switch (effect) {
    case QuarterEffect::DUAL_COMET: {
      int posA = static_cast<int>(progress * AppConfig::NUM_LEDS) % AppConfig::NUM_LEDS;
      int posB = (posA + AppConfig::NUM_LEDS / 2) % AppConfig::NUM_LEDS;
      for (int i = 0; i < 5; i++) {
        uint8_t falloff = static_cast<uint8_t>(25 - i * 4);
        addToPixel(strip, wrap(posA - i), 0, falloff, 18);
        addToPixel(strip, wrap(posB + i), 0, falloff, 18);
      }
      break;
    }
    case QuarterEffect::GREEN_PULSE: {
      float pulse = (sinf(progress * 4.0f * PI) + 1.0f) * 0.5f;
      uint8_t intensity = static_cast<uint8_t>(20 + pulse * 35);
      for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
        if (i % 3 == 0) {
          addToPixel(strip, wrap(i), 0, intensity, 0);
        }
      }
      break;
    }
    default:
      break;
  }
}

void ChimeEffects::applyHourEffect(Adafruit_NeoPixel& strip, float progress) {
  auto effect = static_cast<HourEffect>(activeEffectId);
  switch (effect) {
    case HourEffect::RAINBOW_CHASE: {
      int lead = static_cast<int>(progress * AppConfig::NUM_LEDS * 2) % AppConfig::NUM_LEDS;
      for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
        int distance = abs(i - lead);
        if (distance > AppConfig::NUM_LEDS / 2) {
          distance = AppConfig::NUM_LEDS - distance;
        }
        if (distance < 10) {
          uint8_t wheelPos = static_cast<uint8_t>((i * 256 / AppConfig::NUM_LEDS + static_cast<int>(progress * 255)) & 0xFF);
          uint32_t c = wheel(strip, wheelPos);
          uint8_t r = static_cast<uint8_t>((c >> 16) & 0xFF);
          uint8_t g = static_cast<uint8_t>((c >> 8) & 0xFF);
          uint8_t b = static_cast<uint8_t>(c & 0xFF);
          addToPixel(strip, wrap(i), r / 4, g / 4, b / 4);
        }
      }
      break;
    }
    case HourEffect::AURORA_WAVE: {
      for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
        float phase = progress * 6.0f * PI + static_cast<float>(i) * 0.35f;
        float wave = (sinf(phase) + 1.0f) * 0.5f;
        uint8_t g = static_cast<uint8_t>(20 + wave * 50);
        uint8_t b = static_cast<uint8_t>(10 + (1.0f - wave) * 40);
        addToPixel(strip, wrap(i), 0, g, b);
      }
      break;
    }
    case HourEffect::STAR_BURST: {
      int center = static_cast<int>(progress * AppConfig::NUM_LEDS) % AppConfig::NUM_LEDS;
      int radius = static_cast<int>(progress * (AppConfig::NUM_LEDS / 2));
      for (int i = -radius; i <= radius; i++) {
        int d = abs(i);
        if (d > 15) continue;
        uint8_t intensity = static_cast<uint8_t>(70 - d * 4);
        addToPixel(strip, wrap(center + i), intensity, intensity / 2, 0);
      }
      break;
    }
    default:
      break;
  }
}

void ChimeEffects::apply(Adafruit_NeoPixel& strip) {
  if (!isActive()) return;

  float progress = static_cast<float>(millis() - activeStartMs) / static_cast<float>(activeDurationMs);
  if (progress > 1.0f) progress = 1.0f;

  switch (activeFrequency) {
    case Frequency::MINUTE:
      applyMinuteEffect(strip, progress);
      break;
    case Frequency::QUARTER:
      applyQuarterEffect(strip, progress);
      break;
    case Frequency::HOUR:
      applyHourEffect(strip, progress);
      break;
    default:
      break;
  }

  strip.show();
}
