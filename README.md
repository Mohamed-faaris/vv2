# ESP32 Robot Controller

PlatformIO firmware for ESP32 DevKit V1 with IR + HC-SR04 sensing, L298N motor output, three LEDs, buzzer, built-in LED, Serial CLI, Wi-Fi AP+STA, HTTP dashboard and JSON/CORS API.

## Build
1. Open in VS Code + PlatformIO.
2. Build and upload firmware.
3. Run PlatformIO: Upload Filesystem Image to install `data/index.html`.
4. Serial Monitor: 115200 baud.

## Default Wi-Fi
AP SSID: `ESP32-Robot`; password: `robot1234`; usually `192.168.4.1`. Change credentials before using on a shared network. STA credentials can be set using PATCH `/api/config` (`sta_ssid`, `sta_password`).

## Dashboard and API
`/`, GET `/api/status`, GET `/api/health`, GET/PATCH `/api/config`, POST `/api/config/save`, POST `/api/config/reset`, POST `/api/motor/start`, POST `/api/motor/stop`, GET `/api/logs`, GET `/api/info`.

## CLI
`help`, `status`, `health`, `config show|save|load|reset`, `mode ir|ultrasonic|both|constant`, `log level error|status|info|debug|trace`, `set KEY VALUE`, `motor start|stop`.

## Wiring
IR VCC 3V3 (only if module supports 3.3V), GND, OUT GPIO27. HC-SR04 VCC 5V, GND, TRIG GPIO5, ECHO GPIO18 via 5V-to-3.3V resistor divider. LEDs GPIO25/26/33 through individual 220-330 ohm resistors. Active 3.3V-compatible buzzer GPIO23; use transistor for higher current. L298N IN1 GPIO19, IN2 GPIO21, ENA GPIO22 (remove ENA jumper), OUT1/OUT2 to motor. External motor supply to L298N. All grounds common. Built-in LED GPIO2 (verify board).

## Behaviour
`ir`: IR detection; `ultrasonic`: ultrasonic threshold; `both`: either enabled sensor stops; `constant`: ultrasonic threshold. Stop distance default 5cm. LED bands <= red_limit red, > red_limit and <= yellow_limit yellow, otherwise green. Built-in LED illuminates during safety stop. Ultrasonic timeout in ultrasonic modes is fail-safe stop. All settings persist in NVS when saved; full configuration can be saved via CLI/API.

## Safety
API currently has no authentication; use only on a trusted network. Never connect HC-SR04 ECHO directly to ESP32. Test without motor load before enabling the motor.
