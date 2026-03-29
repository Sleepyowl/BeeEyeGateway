#pragma once
#include <stdint.h>

enum class PixelSignal:uint8_t {
  None = 0,
  Initializing = 1,
  Error = 2,
  Ok = 3,
  Warning = 4
};
void statusLedBegin();
void setPixelSignal(PixelSignal signal, uint8_t brightness = 127);