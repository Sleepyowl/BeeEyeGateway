# BeeEye Gateway

This is the firmware for a custom LoRa gateway based on ESP32 and SX126x. It receives metrics over LoRa and posts them to InfluxDB (over WiFi).

## Building and deploying

- Install Visual Studio Code
- Install PlatformIO extension in Visual Studio Code
- Open the solution
- Open command palette (F1) and select `PlatformIO: Upload`

## Configuring

Device must be configured via serial port.

- Plug the device to your PC
- Connect to the device with serial terminal (for instance, using command `PlatformIO: Serial Monitor`)
- Cofigure settings (use help for list of commands)
- Don't forget to save settings with `save` command


