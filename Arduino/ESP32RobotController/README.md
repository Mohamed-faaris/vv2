# Arduino IDE version — ESP32 Robot Controller

## Open in Arduino IDE
1. Open the folder `Arduino/ESP32RobotController`.
2. Open `ESP32RobotController.ino`.
3. Select **ESP32 Dev Module** (or your matching ESP32 DevKit board).
4. Select the correct COM/serial port and upload.

## Required board package
Install **esp32 by Espressif Systems**, version 3.x, using Boards Manager. This sketch uses the Arduino-ESP32 3.x LEDC API (`ledcAttach`).

## Required libraries
Install these from Library Manager:
- ESPAsyncWebServer (ESP32Async)
- AsyncTCP (ESP32Async)
- ArduinoJson (Benoit Blanchon)

Preferences, WiFi, LittleFS and Arduino are supplied by the ESP32 board package.

## Upload dashboard filesystem
The dashboard is `data/index.html`. Install an ESP32 LittleFS filesystem uploader compatible with your Arduino IDE version, then upload the `data` folder as LittleFS. If you skip this step, the HTTP root page will not be available.

## Default access
AP SSID: `ESP32-Robot`; password: `robot1234`; open `http://192.168.4.1/`.

## Serial CLI
Serial Monitor: 115200 baud. Commands: `help`, `status`, `health`, `mode ir|ultrasonic|both|constant`, `log level error|status|info|debug|trace`, `set KEY VALUE`, `config save`, `config reset`, `motor start|stop`.

## Hardware safety
HC-SR04 ECHO must go through a resistor divider to reduce 5V to 3.3V. Use a common ground between ESP32 and L298N. Use a suitable external motor supply; do not power the motor from the ESP32 3V3 pin. Confirm pin assignments in `pins.h` before wiring. API has no authentication; use only on a trusted network.

## Software-only dashboard additions
- Sensor diagnostics: ultrasonic timeout counter and heuristic IR no-transition warning (not proof of a failed sensor).
- Operating profiles: `safe`, `normal`, and `fast`; presets apply in RAM. Use Configuration → Save to flash to persist them.
- Runtime metrics: obstacle event count, ultrasonic timeout count, IR transitions, motor runtime.
- Event timeline: recent obstacle/path and motor events; stored in RAM and cleared on reboot.
- Diagnostics export: download a JSON snapshot containing status, configuration, metrics, recent events, and logs.

New API routes: `GET /api/metrics`, `GET /api/events`, `GET /api/diagnostics`, `POST /api/profile?name=safe|normal|fast`.
