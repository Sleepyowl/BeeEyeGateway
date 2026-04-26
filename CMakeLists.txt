cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sensor-gateway)

target_sources(app PRIVATE 
    src/main.c
    src/display.c
    src/rtc_clock.c
    src/appconfig.c
    src/shell.c
    src/wifi.c
    src/lora.c
    src/influx_sender.c
    src/font_tom_thumb.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
