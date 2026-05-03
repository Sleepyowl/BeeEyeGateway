#pragma once
#include <stdint.h>

#define PROTOCOL_VERSION 3
#define MAGIC_BEE_EYE 0x45455942ul

#define MESSAGE_TYPE_MEASURE 1
#define MESSAGE_TYPE_TEXT 2

#define MEASURE_TYPE_BATTERY 0
#define MEASURE_TYPE_TEMPERATURE 1
#define MEASURE_TYPE_TEMP_AND_HUMIDITY 2
#define MEASURE_TYPE_WEIGHT 3

#pragma pack(push, 1)
#pragma pack(1)
/// @brief Sensor measure
struct measure {   
    uint8_t         type; 
    uint8_t         sensor_id[8];
    union {
        struct __attribute__((packed)) {
            float tempC;
            float hum;
        } th;

        struct __attribute__((packed)) {
            float weight;
        } w;

        struct __attribute__((packed)) {
            uint16_t mV;
        } bat;
    } data;
}; 

/// @brief Message header
struct message_header {
    uint8_t     magic[4];
    uint8_t     flags; // version && flags    
    uint64_t    srcAddr; 
    uint8_t     type;
};
#pragma pack(pop)
