#pragma once
#include <stdint.h>

#pragma pack(push, 1)
#pragma pack(1)

constexpr uint8_t PROTOCOL_VERSION = 1;
constexpr uint32_t MAGIC_BEYE = 0x45455942ul;


enum class MessageType: uint8_t {
    Measure = 1,     
    Text = 2
};

enum class MeasureType: uint8_t {
    Battery = 0,
    Temperature = 1,
    TemperatureAndHumidity = 2
};

/// @brief Sensor measure
struct Measure {   
    MeasureType     type; 
    uint8_t         sensorAddress[8];
    union {
        uint16_t         mV;
        float           tempC;        
        uint32_t        raw;
    } _data;
    float   hum;
}; 

/// @brief Message header
struct MessageHeader {
    uint8_t     magic[4];
    uint8_t     flags; // version && flags    
    uint64_t    srcAddr; 
    MessageType type;
};
#pragma pack(pop)