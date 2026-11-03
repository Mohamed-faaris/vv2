# ESP32 Robot Controller — Troubleshooting

This guide covers the repository's ESP32 firmware, wiring, build tools, Wi-Fi, Serial CLI, HTTP API, and React dashboard.

> **Feature status:** The checked-in firmware currently supports AP settings and one STA SSID/password. The planned 10-profile Wi-Fi store, Wi-Fi scan/profile CLI/API, and strongest-known-network selection are requirements in `wifi-board-guidience.md`; they are not implemented in the current firmware. Do not assume those commands or endpoints exist.

## 1. Choose the correct firmware project

The repository contains two firmware copies:

| Toolchain | Main source | Headers | Filesystem assets |
|---|---|---|---|
| PlatformIO (repository root) | `src/main.cpp` | `include/pins.h`, `include/config.h` | `data/index.html` |
| Arduino IDE | `Arduino/ESP32RobotController/ESP32RobotController.ino` | `Arduino/ESP32RobotController/pins.h`, `config.h` | `Arduino/ESP32RobotController/data/index.html` |

Edit the intended source tree. If both copies are maintained, keep changes synchronized. The PlatformIO project uses the Arduino framework, LittleFS, ESPAsyncWebServer, AsyncTCP, and ArduinoJson.

## 2. Build and upload with PlatformIO

Run from the repository root:

```bash
pio run
pio run -t upload
pio run -t uploadfs
pio device monitor -b 115200
```

- `pio run`: compile firmware.
- `pio run -t upload`: upload firmware.
- `pio run -t uploadfs`: upload the `data/` directory to LittleFS.
- `pio device monitor -b 115200`: open Serial Monitor.

If `pio` is not found, install PlatformIO Core or use the PlatformIO VS Code extension. Its equivalent actions are **Build**, **Upload**, **Upload Filesystem Image**, and **Monitor**.

For build failures:
1. Read the first compiler error; later errors may be cascading failures.
2. Confirm the target board is an ESP32 DevKit-compatible board.
3. Check that PlatformIO installed the dependencies from `platformio.ini`.
4. Check compatibility between the ESP32 Arduino framework and the async web-server libraries.
5. After correcting the cause, clean and rebuild with `pio run -t clean && pio run`.

## 3. Build and upload with Arduino IDE

1. Open `Arduino/ESP32RobotController/ESP32RobotController.ino`.
2. Select the correct ESP32 board and serial port.
3. Install the libraries documented in `Arduino/ESP32RobotController/README.md`.
4. Compile and upload the sketch.
5. Upload the matching `data/` directory to LittleFS using a filesystem uploader compatible with your Arduino IDE version.
6. Open Serial Monitor at **115200 baud**.

The sketch uses the Arduino-ESP32 3.x LEDC API (`ledcAttach`). An older board package may fail to compile it. Use a compatible version or deliberately adapt PWM calls for the selected core version. Do not mix incompatible ESPAsyncWebServer and AsyncTCP forks.

## 4. Arduino CLI commands

Install Arduino CLI and the ESP32 board core:

```bash
arduino-cli version
arduino-cli config init
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

Install ArduinoJson:

```bash
arduino-cli lib install ArduinoJson
```

Install ESPAsyncWebServer and AsyncTCP from the compatible ESP32Async releases using Library Manager or an appropriate configured Arduino CLI library source. Avoid old forks that provide the same header names.

Find the board and serial port:

```bash
arduino-cli board list
arduino-cli board listall esp32
```

Compile (replace the FQBN if your exact board differs):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 \
  Arduino/ESP32RobotController
```

Upload (replace the port):

```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 \
  Arduino/ESP32RobotController
```

Open Serial Monitor:

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

On Linux the port may instead be `/dev/ttyACM0`; check `arduino-cli board list`. If permission is denied, check that your user can access the serial device (commonly through the `dialout` group), then log out and back in after changing group membership.

**Filesystem limitation:** The ordinary Arduino CLI compile/upload commands above do not upload the LittleFS image. Use a filesystem uploader compatible with the installed toolchain, or use PlatformIO's `pio run -t uploadfs`. Uploading only the sketch does not install the dashboard files.

## 5. Serial Monitor and current CLI

Use **115200 baud**. If output is blank or garbled:
- Use a USB data cable, not a charge-only cable.
- Check the serial port and close other programs that may be using it.
- Check USB-UART drivers and Linux permissions.
- Press EN/RESET after opening the monitor.
- Confirm baud rate is 115200.

Current commands in the firmware include:

| Command | Purpose |
|---|---|
| `help` | Print supported commands |
| `status` | Show mode, motor, IR state, distance, stop reason |
| `health` | Show uptime, free heap, Wi-Fi connected state, sensor-fault state |
| `config` | Show key configuration values |
| `config save` | Save configuration to NVS |
| `config load` | Load saved configuration |
| `config reset` | Restore defaults and save them |
| `mode ir` | Use IR mode |
| `mode ultrasonic` | Use ultrasonic mode |
| `mode both` | Use both sensors |
| `mode constant` | Use ultrasonic threshold mode |
| `log level error` | Error-only logging |
| `log level status` | Status logging |
| `log level info` | Informational logging |
| `log level debug` | Detailed debugging |
| `log level trace` | Most verbose logging |
| `set KEY VALUE` | Change a supported setting |
| `motor start` | Request motor start, subject to safety interlocks |
| `motor stop` | Stop motor |
| `wifi` | Show AP IP and STA connected/disconnected state |

Use `help` from the firmware actually uploaded as the final authority for accepted `set` keys. Runtime changes are not necessarily persistent until `config save` is run.

## 6. Wiring and power checks

Verify the wiring against `include/pins.h` or the Arduino project's corresponding `pins.h`.

| Component | ESP32 pin(s) | Checks |
|---|---|---|
| IR obstacle sensor OUT | GPIO27 | Supply, common ground, output voltage, active-low setting |
| HC-SR04 TRIG | GPIO5 | Trigger wire and common ground |
| HC-SR04 ECHO | GPIO18 through resistor divider | **Never connect 5V ECHO directly to ESP32 GPIO** |
| Red LED | GPIO25 | Series resistor and polarity |
| Yellow LED | GPIO26 | Series resistor and polarity |
| Green LED | GPIO33 | Series resistor and polarity |
| Buzzer | GPIO23 | Polarity and current; use transistor driver when required |
| L298N IN1 / IN2 | GPIO19 / GPIO21 | Direction wiring and common ground |
| L298N ENA | GPIO22 | Remove ENA jumper when using ESP32 PWM |
| Built-in LED | GPIO2 | Verify the board variant uses this pin |

### Motor does not move

1. Disconnect power before changing wiring.
2. Check that the L298N has a suitable external motor supply.
3. Connect ESP32 GND and L298N GND together.
4. Check motor wires at the L298N output terminals.
5. Confirm IN1, IN2, and ENA match `pins.h`.
6. Remove the ENA jumper if GPIO22 is controlling ENA with PWM.
7. Check that the motor is enabled and PWM speed is non-zero.
8. Run `status` and check whether a sensor fault or obstacle safety stop is active.
9. Test with wheels lifted or the motor mechanically unloaded.

Do not power a DC motor from the ESP32 3.3V pin. Do not bypass safety interlocks to force a motor test.

### HC-SR04 times out or reports no distance

- Check 5V supply and common ground.
- Check TRIG=GPIO5 and ECHO=GPIO18 through a suitable divider.
- Ensure the divider output remains within the ESP32 3.3V input range.
- Inspect wires and connectors.
- Test with a flat target at a suitable distance, facing the sensor.
- Soft, narrow, angled, or absorbent objects may return unreliable readings.
- The firmware represents pulse timeout as an invalid distance and sensor fault. In ultrasonic modes this causes a fail-safe stop; it is not a valid 0 cm measurement.

### IR sensor triggers backwards

The default `ir_active_low` setting is true. Some IR modules assert LOW on detection; others assert HIGH. Check the module's output behavior and configure the polarity to match.

### ESP32 resets when the motor starts

This often indicates supply sag or electrical noise:
- Do not power the motor from the ESP32 3.3V pin.
- Use a motor supply rated for startup/stall current.
- Use a suitable regulated supply for the ESP32.
- Keep motor wires away from sensor and antenna wiring.
- Check common ground and supply decoupling.
- Test with the motor disconnected to isolate the power problem.

## 7. Wi-Fi and dashboard access

### Default Access Point

- SSID: `ESP32-Robot`
- Password: `robot1234`
- AP address: normally `192.168.4.1`
- Dashboard: `http://192.168.4.1/`

Connect a phone or laptop to the ESP32 AP and open the dashboard address. This local access point is useful even when no router or internet connection is available. Change the default password before using the board on a shared or untrusted network.

### STA configuration in the current firmware

The checked-in code has one STA SSID/password pair. Configure it with `PATCH /api/config`, then save it:

```bash
curl -i -X PATCH http://192.168.4.1/api/config \
  -H 'Content-Type: application/json' \
  --data '{"sta_ssid":"YOUR_WIFI_NAME","sta_password":"YOUR_WIFI_PASSWORD"}'

curl -i -X POST http://192.168.4.1/api/config/save
```

Replace placeholders locally. Do not commit or paste real credentials into source control, issue comments, shared logs, or screenshots.

**Current firmware limitation:** Wi-Fi interfaces are initialized during boot. The current PATCH handler updates configuration but does not fully reconfigure active Wi-Fi interfaces immediately. Restart the ESP32 after changing AP/STA settings. An API success response does not prove that STA has connected.

The current firmware does not yet implement 10 saved profiles, Wi-Fi scan/profile CLI/API operations, strongest-known-network selection, or explicit saved-profile connections.

### ESP32 AP is not visible

- Move close to the board and scan again.
- Check Serial boot output for AP startup and IP.
- Check whether Wi-Fi was disabled in saved settings.
- Press reset and inspect the boot logs.
- Check for repeated resets due to poor power.
- Use a 2.4 GHz-capable client; classic ESP32 boards do not connect to 5 GHz-only Wi-Fi.

### STA does not connect

- Verify the SSID and password carefully.
- Confirm the router offers a supported 2.4 GHz network.
- Check router security mode and DHCP availability.
- Confirm STA is enabled and the configuration has been saved.
- Restart the ESP32 after changing STA settings.
- Use Serial `wifi` and `health` to inspect the reported state.
- Use the AP address as the local recovery route if router connection fails.

## 8. HTTP API tests with curl

Run these while connected to the ESP32 AP or while the ESP32 has a reachable STA address.

### Read status, health, information, configuration, and logs

```bash
curl -i http://192.168.4.1/api/status
curl -i http://192.168.4.1/api/health
curl -i http://192.168.4.1/api/info
curl -i http://192.168.4.1/api/config
curl -i http://192.168.4.1/api/logs
```

Expected behavior:
- `/api/status`: motor, sensors, outputs, mode, and uptime.
- `/api/health`: health result; current firmware returns HTTP 503 when the ultrasonic sensor fault is set.
- `/api/info`: firmware, chip, and runtime information.
- `/api/config`: current non-secret configuration. Passwords are intentionally omitted.
- `/api/logs`: bounded recent log buffer.

### Update a setting and persist it

```bash
curl -i -X PATCH http://192.168.4.1/api/config \
  -H 'Content-Type: application/json' \
  --data '{"mode":"both","stop_distance":8,"motor_speed":160}'

curl -i -X POST http://192.168.4.1/api/config/save
```

PATCH validates the configuration update. Save it separately to persist across restart.

### Motor control

```bash
curl -i -X POST http://192.168.4.1/api/motor/stop
curl -i -X POST http://192.168.4.1/api/motor/start
```

The start endpoint should reject a request while the safety interlock is active. Keep the motor mechanically safe during testing.

### Reset configuration

```bash
curl -i -X POST http://192.168.4.1/api/config/reset
```

This restores firmware defaults and saves them. It is not a device restart.

## 9. Dashboard connection failures

- Confirm the computer and ESP32 are on the same reachable network.
- Confirm the API base URL. AP default is `http://192.168.4.1`.
- Test `/api/health` using curl before investigating React.
- Confirm the firmware booted and the HTTP server started.
- Upload the LittleFS filesystem image; otherwise API routes may work while the dashboard root page is missing.
- Use browser developer tools to inspect failed requests and CORS errors.

### HTTPS-to-HTTP mixed content

A dashboard served over HTTPS cannot normally call the ESP32's plain HTTP API from browser JavaScript. Browsers block this as mixed active content. Use one of these approaches:
- Serve the dashboard from ESP32 over local HTTP.
- Run a local development server over HTTP on a network that can reach the ESP32.
- Use a properly secured HTTPS proxy/tunnel between the hosted dashboard and the device.

Do not disable browser security as a production workaround.

### CORS is not security

CORS controls which browser origins may read responses. It does not authenticate users or protect motor/configuration endpoints. The current firmware API has no authentication; use it only on a trusted isolated network.

## 10. Common symptoms and likely causes

| Symptom | Likely cause | Action |
|---|---|---|
| Compile error at `ledcAttach` | Arduino-ESP32 core version mismatch | Use the documented compatible core or adapt PWM code deliberately |
| HTTP 404 | Wrong route or route not implemented | Check exact path and uploaded firmware version |
| HTTP 400 | Invalid JSON or value out of range | Inspect response body and verify field names/types |
| HTTP 409 on motor start | Safety interlock active | Inspect `/api/status` and sensor fault state |
| HTTP 503 on health | Ultrasonic timeout/fault | Check sensor supply, ECHO divider, wiring, target |
| CORS failure in browser | Origin/preflight/header mismatch | Test with curl and inspect OPTIONS behavior |
| Connection refused | Wrong IP, HTTP server not started, board offline | Check AP/STA IP and Serial boot log |
| Dashboard blank | LittleFS image missing or wrong path | Upload filesystem and inspect serial output |
| STA config changed but old connection remains | Wi-Fi initialized at boot in current code | Save config and restart ESP32 |

## 11. Safe diagnostic information to collect

Before reporting a problem, collect:
1. Firmware path: PlatformIO or Arduino IDE.
2. Exact board model and ESP32 Arduino core version.
3. Build command and first compiler error, if any.
4. Serial boot log with secrets removed.
5. Output of `help`, `status`, `health`, and `wifi`.
6. Responses from `/api/health` and `/api/status`.
7. Wiring description or clear photo.
8. Exact reproduction steps and whether the fault remains with the motor disconnected.

Never include Wi-Fi passwords, access tokens, or private keys in bug reports.

## 12. Security and intended use

The current firmware API does not implement authentication. Use it only on a trusted local network. Do not port-forward it or expose it directly to the internet. Change default Wi-Fi credentials, keep diagnostics free of secrets, and add authentication before using the device on a shared network.
