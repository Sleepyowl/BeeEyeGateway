#pragma once
#include <WString.h>


bool displayBegin();
void displayPrint(const String& text);
void displayPrintf(const char* format, ...);
void displayFatalError(const char* error);
void drawStatusBar(bool wifi);
void displayTime(uint8_t hour, uint8_t minute, bool showDots, bool wifiConnected, float temp, uint8_t fontSize = 4);