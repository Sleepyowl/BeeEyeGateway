#include "LedStatus.h"

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(1, 8, NEO_GRB + NEO_KHZ800);
void setPixelSignal(PixelSignal signal, uint8_t brightness) {
  auto color = pixels.Color(0,0,0);
  switch(signal) {
    case PixelSignal::Initializing:
      color = pixels.Color(0,0, brightness);
      break;
    case PixelSignal::Error:
      color = pixels.Color(brightness,0,0);
      break;
    case PixelSignal::Ok:
      color = pixels.Color(0,brightness,0);
      break;
    case PixelSignal::Warning:
      color = pixels.Color(brightness,brightness,0);
      break;
    case PixelSignal::None:
    default:
      color = pixels.Color(0,0,0);
  }
  pixels.setPixelColor(0, color);
  pixels.show();
}

void statusLedBegin() {
    pixels.begin();
}