#include "clock_renderer.h"

#include <cmath>

ClockRenderer::ClockRenderer(Adafruit_NeoPixel& stripRef) : strip(stripRef) {}

void ClockRenderer::begin() {
  lastSecChange = millis();
  lastMinChange = millis();
  lastHourChange = millis();
}

void ClockRenderer::nextMode() {
  currentMode = static_cast<AppConfig::ClockMode>((currentMode + 1) % AppConfig::MODE_COUNT);
}

AppConfig::ClockMode ClockRenderer::selectedMode() const {
  return currentMode;
}

const char* ClockRenderer::selectedModeName() const {
  return AppConfig::MODE_NAMES[currentMode];
}

int ClockRenderer::wrap(int position) {
  return (((position + AppConfig::LED_OFFSET) % AppConfig::NUM_LEDS) + AppConfig::NUM_LEDS) % AppConfig::NUM_LEDS;
}

float ClockRenderer::breathe(unsigned long periodMs) {
  float t = static_cast<float>(millis() % periodMs) / static_cast<float>(periodMs);
  return (sinf(t * 2.0f * PI - PI / 2.0f) + 1.0f) / 2.0f;
}

float ClockRenderer::expDecay(int distance, float rate) {
  return powf(rate, static_cast<float>(distance));
}

float ClockRenderer::easeIn(unsigned long startMs, unsigned long durationMs) {
  unsigned long elapsed = millis() - startMs;
  if (elapsed >= durationMs) return 1.0f;
  float t = static_cast<float>(elapsed) / static_cast<float>(durationMs);
  return t * t;
}

float ClockRenderer::easeOut(unsigned long startMs, unsigned long durationMs) {
  unsigned long elapsed = millis() - startMs;
  if (elapsed >= durationMs) return 0.0f;
  float t = 1.0f - static_cast<float>(elapsed) / static_cast<float>(durationMs);
  return t * t;
}

bool ClockRenderer::isNight(int h24) const {
  return (AppConfig::NIGHT_START > AppConfig::NIGHT_END)
    ? (h24 >= AppConfig::NIGHT_START || h24 < AppConfig::NIGHT_END)
    : (h24 >= AppConfig::NIGHT_START && h24 < AppConfig::NIGHT_END);
}

float ClockRenderer::subSecond() const {
  unsigned long elapsed = millis() - lastSecChange;
  float value = static_cast<float>(elapsed) / 1000.0f;
  return (value > 0.999f) ? 0.999f : value;
}

void ClockRenderer::clearBuf() {
  memset(bufR, 0, sizeof(bufR));
  memset(bufG, 0, sizeof(bufG));
  memset(bufB, 0, sizeof(bufB));
}

void ClockRenderer::addPixel(int pos, uint16_t r, uint16_t g, uint16_t b) {
  int wrapped = wrap(pos);
  bufR[wrapped] += r;
  bufG[wrapped] += g;
  bufB[wrapped] += b;
}

void ClockRenderer::layerSciFiGrid() {
  float pulse = breathe(8000);
  for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
    if (i % 5 == 0) {
      addPixel(i, 5, 5, 8);
    } else {
      addPixel(i, 0, 0, static_cast<uint16_t>(3.0f * pulse));
    }
  }
}

void ClockRenderer::layerSecondProgress(int sec) {
  for (int i = 0; i < sec; i++) {
    addPixel(i, 0, 0, 6);
  }
}

void ClockRenderer::layerMinuteProgress(int minute) {
  for (int i = 0; i < minute; i++) {
    addPixel(i, 0, 3, 0);
  }
}

void ClockRenderer::layerHours(int hourPos, AppConfig::ClockMode mode) {
  int arcHalf = (mode == AppConfig::MODE_NIGHT) ? 0 : 2;
  int maxBrightness = (mode == AppConfig::MODE_NIGHT) ? 160 : 220;

  for (int i = -arcHalf; i <= arcHalf; i++) {
    float brightness = 1.0f - fabsf(static_cast<float>(i)) * 0.3f;
    if (brightness < 0.0f) brightness = 0.0f;
    addPixel(hourPos + i, static_cast<uint16_t>(maxBrightness * brightness), 0, 0);
  }
}

void ClockRenderer::layerHoursFade() {
  if (fadeOutHourPos < 0) return;
  float fade = easeOut(lastHourChange, 1000);
  if (fade > 0.01f) {
    addPixel(fadeOutHourPos, static_cast<uint16_t>(100 * fade), 0, 0);
  }
}

void ClockRenderer::layerMinutes(int minPos, AppConfig::ClockMode mode, float easeF) {
  float breatheFactor = 1.0f;
  int maxBrightness = 220;

  switch (mode) {
    case AppConfig::MODE_CLASSIC:
      break;
    case AppConfig::MODE_NIGHT:
      breatheFactor = 0.5f + 0.5f * breathe(6000);
      maxBrightness = 130;
      break;
    case AppConfig::MODE_AMBIENT:
      breatheFactor = 0.6f + 0.4f * breathe(4000);
      maxBrightness = 200;
      break;
    default:
      breatheFactor = 0.7f + 0.3f * breathe(4000);
      break;
  }

  uint16_t green = static_cast<uint16_t>(maxBrightness * breatheFactor * easeF);
  addPixel(minPos, 0, green, 0);

  if (mode != AppConfig::MODE_NIGHT && mode != AppConfig::MODE_CLASSIC) {
    uint16_t side = static_cast<uint16_t>(green * 0.3f);
    addPixel(minPos - 1, 0, side, 0);
    addPixel(minPos + 1, 0, side, 0);
  }
}

void ClockRenderer::layerMinutesFade() {
  if (fadeOutMinPos < 0) return;
  float fade = easeOut(lastMinChange, 500);
  if (fade > 0.01f) {
    addPixel(fadeOutMinPos, 0, static_cast<uint16_t>(120 * fade), 0);
  }
}

void ClockRenderer::layerSeconds(int sec, AppConfig::ClockMode mode, float easeF) {
  float subSec = subSecond();

  switch (mode) {
    case AppConfig::MODE_CLASSIC:
      addPixel(sec, 0, 0, static_cast<uint16_t>(220 * easeF));
      break;

    case AppConfig::MODE_COMET: {
      float headPos = static_cast<float>(sec) + subSec;
      int headLed = static_cast<int>(headPos) % AppConfig::NUM_LEDS;
      float frac = headPos - static_cast<int>(headPos);

      uint16_t headBrightness = static_cast<uint16_t>(240 * easeF);
      addPixel(headLed, 0, 0, static_cast<uint16_t>(headBrightness * (1.0f - frac)));
      if (frac > 0.05f) {
        addPixel(headLed + 1, 0, 0, static_cast<uint16_t>(headBrightness * frac));
      }

      for (int i = 1; i <= 5; i++) {
        float brightness = expDecay(i, 0.42f) * easeF;
        addPixel(headLed - i, 0, 0, static_cast<uint16_t>(240 * brightness));
      }
      break;
    }

    case AppConfig::MODE_AMBIENT: {
      float breatheFactor = 0.8f + 0.2f * breathe(1000);
      addPixel(sec, 0, 0, static_cast<uint16_t>(200 * breatheFactor * easeF));
      addPixel(sec - 1, 0, 0, static_cast<uint16_t>(70 * breatheFactor * easeF));
      addPixel(sec + 1, 0, 0, static_cast<uint16_t>(35 * breatheFactor * easeF));
      break;
    }

    case AppConfig::MODE_NIGHT:
      addPixel(sec, 0, 0, static_cast<uint16_t>(100 * easeF));
      break;

    case AppConfig::MODE_SCIFI: {
      float headPos = static_cast<float>(sec) + subSec;
      int headLed = static_cast<int>(headPos) % AppConfig::NUM_LEDS;
      addPixel(headLed, 0, 0, static_cast<uint16_t>(220 * easeF));
      for (int i = 1; i <= 3; i++) {
        float brightness = expDecay(i, 0.32f) * easeF;
        addPixel(headLed - i, 0, 0, static_cast<uint16_t>(220 * brightness));
      }
      break;
    }

    default:
      break;
  }
}

void ClockRenderer::layerSecondsFade() {
  if (fadeOutSecPos < 0) return;
  float fade = easeOut(lastSecChange, 150);
  if (fade > 0.01f) {
    addPixel(fadeOutSecPos, 0, 0, static_cast<uint16_t>(150 * fade));
  }
}

void ClockRenderer::layerTick(int sec, AppConfig::ClockMode mode) {
  if (mode != AppConfig::MODE_NIGHT && millis() - lastSecChange < 30) {
    addPixel(sec, 8, 8, 25);
  }
}

void ClockRenderer::fuseAndOutput(uint8_t brightness) {
  float scale = brightness / 255.0f;

  for (int i = 0; i < AppConfig::NUM_LEDS; i++) {
    uint8_t r = (bufR[i] > 255) ? 255 : static_cast<uint8_t>(bufR[i]);
    uint8_t g = (bufG[i] > 255) ? 255 : static_cast<uint8_t>(bufG[i]);
    uint8_t b = (bufB[i] > 255) ? 255 : static_cast<uint8_t>(bufB[i]);

    bool hasR = (r > 40);
    bool hasG = (g > 40);
    bool hasB = (b > 40);
    if (hasR && hasG && hasB) {
      uint8_t peak = r;
      if (g > peak) peak = g;
      if (b > peak) peak = b;
      uint8_t soft = static_cast<uint8_t>(peak * 0.65f);
      r = soft;
      g = soft;
      b = soft;
    }

    if (r > 230) r = 230;
    if (g > 230) g = 230;
    if (b > 230) b = 230;

    strip.setPixelColor(i,
      static_cast<uint8_t>(r * scale),
      static_cast<uint8_t>(g * scale),
      static_cast<uint8_t>(b * scale));
  }

  strip.show();
}

void ClockRenderer::render(const tm& localTime) {
  int sec = localTime.tm_sec;
  int minute = localTime.tm_min;
  int h12 = localTime.tm_hour % 12;
  int h24 = localTime.tm_hour;
  int hourPos = (h12 * 5 + minute / 12) % AppConfig::NUM_LEDS;

  if (sec != prevSec) {
    fadeOutSecPos = prevSec;
    lastSecChange = millis();
    prevSec = sec;
  }
  if (minute != prevMin) {
    fadeOutMinPos = prevMin;
    lastMinChange = millis();
    prevMin = minute;
  }
  if (hourPos != prevHourPos) {
    fadeOutHourPos = prevHourPos;
    lastHourChange = millis();
    prevHourPos = hourPos;
  }

  bool night = isNight(h24);
  AppConfig::ClockMode renderMode = night ? AppConfig::MODE_NIGHT : currentMode;
  uint8_t brightness = night ? AppConfig::NIGHT_BRIGHTNESS : AppConfig::DAY_BRIGHTNESS;

  float secEase = easeIn(lastSecChange, 150);
  float minEase = easeIn(lastMinChange, 500);

  clearBuf();

  if (renderMode == AppConfig::MODE_SCIFI) {
    layerSciFiGrid();
  }

  if (renderMode == AppConfig::MODE_COMET || renderMode == AppConfig::MODE_SCIFI || renderMode == AppConfig::MODE_AMBIENT) {
    layerSecondProgress(sec);
  }
  if (renderMode == AppConfig::MODE_AMBIENT) {
    layerMinuteProgress(minute);
  }

  layerHours(hourPos, renderMode);
  layerHoursFade();

  layerMinutes(minute, renderMode, minEase);
  layerMinutesFade();

  layerSeconds(sec, renderMode, secEase);
  layerSecondsFade();

  layerTick(sec, renderMode);

  fuseAndOutput(brightness);
}
