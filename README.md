# BeeEye Gateway

This is the firmware for a custom LoRa gateway based on ESP32 and SX126x. It receives metrics over LoRa and posts the to InfluxDB (over WiFi).

## Building and deploying

- Install Visual Studio Code
- Install PlatformIO extension in Visual Studio Code
- Open the solution
- Open command palette (F1) and select `PlatformIO: Upload`

## Configuring

- Connect to the device with serial terminal (for instance, using command `PlatformIO: Serial Monitor`)
- Cofigure settings (use help for list of commands)
- Don't forget to save settings with `save` command


