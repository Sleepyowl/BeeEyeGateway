#include "Wiring.h"

#include <pgmspace.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace Icons {
    const uint8_t WiFi[8] PROGMEM = {
        0b00000000,
        0b00000000,
        0b01111110,
        0b10000001,
        0b00111100,
        0b01000010,
        0b00011000,
        0b00011000
        };

    const uint8_t degree[8] PROGMEM = {
        0b00111000,
        0b00101000,
        0b00111000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
    };
}
Adafruit_SSD1306 display(
    Wiring::OledDisplay::ScreenWidth,
    Wiring::OledDisplay::ScreenHeight,
    &Wire, 
    Wiring::OledDisplay::ResetPin);


void scrollDisplayUp() {
  // assumes 128x64 buffer (8 pages × 128 bytes)
  uint8_t* buf = display.getBuffer();
  // move everything up by 8 pixels (1 page = 128 bytes)
  memmove(buf, buf + 128, 128 * 7);
  // clear the last 8-pixel-high line
  memset(buf + 128 * 7, 0, 128);
  //display.display();
}

void displayPrint(const String& text) {
  const uint8_t textSize = 1;
  const auto textLength = text.length();
  const auto linesNeeded = 1 + ((textLength * 6 * textSize - 1) / display.width());
  for(int i = 0; i < linesNeeded; ++i) {
    scrollDisplayUp();
  }
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, display.height() - linesNeeded * 8);
  display.print(text);  
  display.display();
}

void displayPrintf(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  displayPrint(buf);
  va_end(args);
}

void displayFatalError(const char* error) {
  const auto message = String("[FATAL] ") + error;
  displayPrint(message);
  Serial.println(message);
}

void drawStatusBar(bool wifi, float temp) {
    // Clear top line
    uint8_t* b = display.getBuffer();
    memset(b, 0, display.width());

    // Draw status icons
    if(wifi) { 
        display.drawBitmap(display.width() - 8, 0, Icons::WiFi, 8, 8, SSD1306_WHITE);
    }

    if(temp >= -273.15) {
      display.setTextSize(1);
      display.setCursor(0,0);
      display.printf("%.1f", temp);
      display.drawBitmap(display.getCursorX(), display.getCursorY(), Icons::degree, 6, 8, SSD1306_WHITE);
      display.setCursor(display.getCursorX() + 7, display.getCursorY());
      display.print("C");
    }
}

void displayTime(uint8_t hour, uint8_t minute, bool showDots, bool wifiConnected, float temp, uint8_t fontSize) {  
  display.clearDisplay();
  display.dim(hour < 7 && hour > 20);
  display.setTextSize(fontSize);  
  display.setCursor((display.width() - 5 * (fontSize * 6)) / 2, (display.height() - fontSize*8) / 2);
  display.printf("%02d%s%02d", hour, showDots ? ":" : " ", minute);
  drawStatusBar(wifiConnected, temp);
  display.display();
}

bool displayBegin() {
    return display.begin(SSD1306_SWITCHCAPVCC, Wiring::OledDisplay::Address);
}
