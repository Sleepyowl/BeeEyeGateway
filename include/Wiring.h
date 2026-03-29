#pragma once
#include <stdint.h>

namespace Wiring {
    namespace OledDisplay {
        constexpr uint8_t ScreenWidth = 128;
        constexpr uint8_t ScreenHeight = 64;
        constexpr uint8_t ResetPin = -1;
        constexpr uint8_t Address = 0x3C;
    }

    namespace SX1262 {
        constexpr float FREQ    = 868.1;
        constexpr uint8_t CS    = 0;
        constexpr uint8_t RST   = 1;
        constexpr uint8_t SCK   = 2;
        constexpr uint8_t MOSI  = 3;
        constexpr uint8_t MISO  = 4;
        constexpr uint8_t DIO1  = 18;
        constexpr uint8_t BUSY  = 15;
        constexpr uint8_t RFSW  = 14;
    }

    constexpr uint8_t SDA = 19;
    constexpr uint8_t SCL = 20;
}