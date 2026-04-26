# BeeEye Gateway

Firmware for a sensor data forwarder based on ESP32-C6 and SX126x.

Receives metrics over LoRa and sends them to InfluxDB over WiFi.

Configuration is done over UART.

## Build and deploy

Requires Zephyr SDK.

Install it by following the official guide:
https://docs.zephyrproject.org/latest/develop/getting_started/index.html

Build and flash:
```bash
west build --pristine --board base_station//hpcore
west flash
```

## Configure

Configuration is done via serial shell.

Connect the device to a USB port on your PC and open a serial terminal:
```bash
tio /dev/ttyACM0 -b 115200
```

Set up WiFi:
```
wifi cred add -s <SSID> -p <PASS> -k 1
mgr wifissid <SSID>
mgr save
```

Set up InfluxDB:
```
mgr influxurl http://your.instance/write?db=target_db
mgr influxuser your_influx_user
mgr influxpass your_influx_pass
mgr save
```

Restart the device with `mgr restart` or by reconnecting USB.