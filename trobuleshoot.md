# Project Troubleshooting Guide

This guide covers the ESP32 Robot Controller repository, its hardware setup, firmware build and upload, Wi-Fi access, HTTP API, frontend dashboard and Vercel deployment.

> **Important:** This document reflects the repository configuration and firmware interfaces documented in the project at the time it was written. The 10-network Wi-Fi profile manager and full Wi-Fi CLI/API management described in `wifi-board-guidience.md` are target requirements, not capabilities to assume are already implemented. Check the current source before relying on a newly requested feature.

## 1. Identify the software being built

The repository contains more than one firmware/frontend area:

- **Main PlatformIO firmware:** root-level `platformio.ini`, `src/`, `include/` and `data/`.
- **Arduino IDE firmware copy:** `Arduino/ESP32RobotController/`.
- **React dashboard:** `frontend/`.

For normal PlatformIO development, open the repository root as the PlatformIO project. Do not accidentally build the Arduino IDE sketch when intending to build the root PlatformIO project.

The PlatformIO configuration currently specifies:
- Board: `esp32dev`
- Framework: Arduino
- Filesystem: LittleFS
- Serial monitor speed: 115200
- Libraries: ESPAsyncWebServer, AsyncTCP and ArduinoJson

The Arduino IDE folder has its own README and dependency instructions. Keep the two firmware copies synchronized deliberately; do not assume editing one automatically updates the other.

## 2. Hardware reference and wiring checks

Confirm the actual board and wiring before debugging firmware. Pin assignments below reflect the current project configuration; verify `include/pins.h` (PlatformIO) or `Arduino/ESP32RobotController/pins.h` (Arduino IDE) before changing wiring.

| Component | ESP32 GPIO / connection | Troubleshooting notes |
|---|---|---|
| IR obstacle sensor OUT | GPIO 27 | Confirm output logic and whether the sensor is active LOW |
| HC-SR04 TRIG | GPIO 5 | ESP32 output to sensor trigger |
| HC-SR04 ECHO | GPIO 18 through a resistor divider | **Never connect 5V ECHO directly to ESP32 GPIO** |
| Red LED | GPIO 25 through resistor | Use an individual 220–330 Ω series resistor |
| Yellow LED | GPIO 26 through resistor | Use an individual 220–330 Ω series resistor |
| Green LED | GPIO 33 through resistor | Use an individual 220–330 Ω series resistor |
| Buzzer | GPIO 23 | Confirm it is compatible with 3.3V GPIO; use a transistor driver if current is too high |
| L298N IN1 | GPIO 19 | Check wiring and motor direction |
| L298N IN2 | GPIO 21 | Check wiring and motor direction |
| L298N ENA | GPIO 22 | Remove the ENA jumper when controlling ENA from the ESP32 |
| Built-in LED | GPIO 2 | Some ESP32 boards use a different built-in LED pin |

### Electrical safety

- Connect ESP32 GND, sensor GND and L298N GND together.
- Power the motor from a suitable external motor supply connected to the L298N. Do not power the motor from the ESP32 3.3V pin.
- Confirm motor supply voltage and current are appropriate for the motor and driver.
- Use the HC-SR04 ECHO resistor divider to reduce the 5V signal to a safe ESP32 input level.
- Check LED polarity and series resistors.
- Disconnect motor power while debugging sensor readings or uploading firmware.
- Test the motor with wheels raised or the motor mechanically unloaded before normal operation.

## 3. Firmware build and upload (PlatformIO)

From the repository root:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

If the `pio` command is not found, install PlatformIO Core or use the PlatformIO extension in VS Code and run the equivalent commands from its integrated terminal.

Upload the dashboard filesystem separately:

```bash
pio run -t uploadfs
```

The root project uses LittleFS (`board_build.filesystem = littlefs`) and serves the dashboard file from the `data/` directory. A successful firmware upload does not automatically mean the filesystem image was uploaded.

### Common build problems

**Missing library / header**
- Confirm PlatformIO is building from the repository root.
- Check that PlatformIO can download the dependencies listed in `platformio.ini`.
- Do not mix incompatible ESPAsyncWebServer and AsyncTCP library families.
- Remove stale build artifacts only when needed, then rebuild:

```bash
pio run -t clean
pio run
```

**LEDC API compile errors**
- The Arduino IDE firmware documentation specifies Arduino-ESP32 3.x and uses the 3.x LEDC API (`ledcAttach`).
- Check the installed Espressif ESP32 board package version. Do not blindly replace LEDC calls without checking the installed core API.

**Upload fails or no serial port appears**
- Check USB data cable (some cables are charge-only).
- Check Linux serial permissions and whether another program has the port open.
- Select the correct port and board.
- Try holding BOOT while the upload begins if the board requires it.
- Disconnect external circuitry that may interfere with boot strapping pins or serial signals.

**Dashboard page is blank / 404 after upload**
- Confirm `data/index.html` exists in the PlatformIO project.
- Run the filesystem upload command.
- Check Serial Monitor for LittleFS mount errors.
- Confirm the firmware and filesystem are being uploaded to the same physical ESP32.

## 4. Arduino IDE build and upload

For the Arduino IDE copy:

1. Open `Arduino/ESP32RobotController/ESP32RobotController.ino`.
2. Select the matching ESP32 board (the project README references ESP32 Dev Module).
3. Install the Espressif ESP32 board package version documented by that folder's README (3.x).
4. Install ESPAsyncWebServer, AsyncTCP and ArduinoJson using compatible ESP32Async libraries.
5. Upload the sketch.
6. Upload the LittleFS filesystem image containing `data/index.html` using an uploader compatible with the installed Arduino IDE.

If the Serial Monitor shows a firmware boot message but the root dashboard page is unavailable, check the LittleFS upload separately.

## 5. Serial CLI troubleshooting

Open the Serial Monitor at **115200 baud**. Send commands terminated by a newline.

Documented commands include:

```text
help
status
health
config show
config save
config load
config reset
mode ir
mode ultrasonic
mode both
mode constant
log level error
log level status
log level info
log level debug
log level trace
set KEY VALUE
motor start
motor stop
wifi
```

Use `help` on the running firmware as the definitive list for that build. The current documented `wifi` command reports Wi-Fi information; it should not be mistaken for a full profile manager with scan/add/remove/connect commands.

### No Serial output

- Confirm the correct serial port and 115200 baud.
- Press EN/RESET and watch for boot output.
- Check USB cable, port permissions and whether another serial monitor has the port open.
- Confirm the board has power and is not held in bootloader mode.

### Garbled Serial output

- Set the monitor to 115200 baud.
- Check that the application and monitor baud rates match.
- Avoid multiple tools opening the same serial device simultaneously.

### Unknown command

- Run `help`.
- Send one command at a time.
- Avoid unsupported aliases or extra punctuation.
- Check that the command is available in the firmware version actually flashed to the board.

## 6. Default Wi-Fi access

The documented default AP settings are:

- SSID: `ESP32-Robot`
- Password: `robot1234`
- Default AP address: `192.168.4.1`
- Dashboard: `http://192.168.4.1/`

Connect a phone or computer to the ESP32 access point, then open the address above.

Some phones display **“Connected without internet”** or automatically switch back to mobile data. This is expected because the ESP32 access point is intended for local device access and may not provide internet. Keep the device connected to the ESP32 network while opening the dashboard.

Change the default AP password before using the board in a shared or untrusted environment.

## 7. AP, STA and Wi-Fi connection issues

The project configuration includes AP and STA enable flags. The current configuration model also has one `sta_ssid` and one `sta_password` field. This is not the same as a 10-profile saved-network manager.

### AP network does not appear

- Confirm Wi-Fi is enabled and AP mode is enabled in the active configuration.
- Reboot and check Serial logs for AP startup errors.
- Verify the correct SSID and password.
- Move close to the ESP32 and check for local 2.4 GHz interference.
- Confirm the AP password is valid for the selected security mode.
- Check whether another nearby network is using the same SSID.

### Cannot connect to the AP

- Re-enter the password exactly; it is case-sensitive.
- Forget the saved network on the phone/computer and reconnect.
- Check whether the AP credentials were changed and persisted.
- Keep in mind that an open or incorrectly configured AP may behave differently from a password-protected AP.

### STA does not connect to a router

- Confirm the router provides a 2.4 GHz network supported by the ESP32. Typical ESP32 boards do not connect to 5 GHz-only Wi-Fi.
- Check the SSID and password carefully.
- Check router access controls, MAC filtering, DHCP availability and client limits.
- Move the board closer to the router.
- Check Serial output and the reported STA connection state.
- Do not assume that an accepted configuration request means the ESP32 has connected successfully.

### AP works but STA does not

This can be normal when AP and STA are enabled but no valid station credentials are configured, or when the router is unavailable. Verify the STA SSID/password and inspect the connection state separately from AP status.

### Current Wi-Fi feature limitations to remember

The existing configuration model documents a single STA SSID/password pair. The current Serial command list documents a Wi-Fi status command, not a complete Wi-Fi profile manager. Before relying on saved profiles, strongest-known-network selection, explicit saved-profile selection, or CLI/API credential management, verify that these features have actually been implemented in the firmware being tested.

## 8. Sensor troubleshooting

### IR sensor always reports obstacle / always clear

- Check sensor VCC and GND.
- Verify the OUT wire is connected to GPIO 27.
- Confirm the sensor output voltage is safe for ESP32 GPIO.
- Adjust the sensor's onboard potentiometer if it has one.
- Check whether the module is active LOW or active HIGH. The default configuration documents active LOW.
- Test the sensor independently before enabling motor control.

### HC-SR04 reports invalid distance or timeout

- Confirm VCC is 5V and GND is common with the ESP32.
- Confirm TRIG is GPIO 5 and ECHO is GPIO 18 through a resistor divider.
- Never connect ECHO directly to the ESP32 because it can be 5V.
- Check TRIG/ECHO are not swapped.
- Ensure the target is within the sensor's practical range and has a surface that reflects ultrasound.
- Avoid testing against angled, soft or very narrow objects.
- Check the configured ultrasonic timeout. The documented default is 30 ms.
- A timeout is treated as a sensor fault and fail-safe stop in the current control logic; do not interpret it as a valid zero-distance reading.

### Distance fluctuates

- Check power stability and common ground.
- Keep sensor wiring away from motor wiring.
- Avoid readings while the motor causes electrical noise.
- Ensure the sensor is mounted rigidly and aimed correctly.
- Use repeated samples or filtering only if it does not weaken obstacle safety.

## 9. Motor, LED and buzzer troubleshooting

### Motor does not run

1. Disconnect or raise the motor load before testing.
2. Check external motor power and common ground.
3. Confirm L298N IN1, IN2 and ENA wiring.
4. If GPIO 22 controls ENA, remove the L298N ENA jumper.
5. Confirm the motor is enabled in configuration and speed is non-zero.
6. Check whether the safety interlock is active due to an obstacle or sensor timeout.
7. Use `status` and `health` before trying `motor start`.
8. Do not bypass the safety interlock to force the motor to run.

### Motor runs in the wrong direction

- Power down before changing wiring.
- Check IN1/IN2 direction mapping and motor polarity.
- Change direction in firmware or swap motor leads as appropriate.
- Re-test with the motor unloaded.

### LEDs do not work

- Verify the GPIO mapping in `pins.h`.
- Check LED polarity.
- Confirm each LED has its own series resistor.
- Verify the corresponding output is enabled in configuration.

### Buzzer does not work

- Check buzzer type, polarity, voltage and current.
- Confirm the GPIO 23 connection.
- Use a transistor driver if the buzzer requires more current than an ESP32 pin can supply.
- Check buzzer enable and stop-alert configuration.

## 10. HTTP API checks

The documented API routes are:

| Method | Route | Purpose |
|---|---|---|
| GET | `/api/status` | Current device, sensor, motor and output state |
| GET | `/api/health` | Health summary |
| GET | `/api/config` | Current non-secret configuration |
| PATCH | `/api/config` | Update supported configuration values |
| POST | `/api/config/save` | Persist current configuration |
| POST | `/api/config/reset` | Restore defaults |
| POST | `/api/motor/start` | Request motor start, subject to safety interlocks |
| POST | `/api/motor/stop` | Stop motor |
| GET | `/api/logs` | Recent log buffer |
| GET | `/api/info` | Device and firmware information |

Example checks from a computer connected to the ESP32 AP:

```bash
curl -i http://192.168.4.1/api/health
curl -i http://192.168.4.1/api/status
curl -i http://192.168.4.1/api/config
curl -i http://192.168.4.1/api/logs
curl -i http://192.168.4.1/api/info
```

Example configuration update:

```bash
curl -i -X PATCH http://192.168.4.1/api/config \
  -H 'Content-Type: application/json' \
  -d '{"mode":"both","stop_distance":8,"motor_speed":160}'
```

Save configuration after verifying the values:

```bash
curl -i -X POST http://192.168.4.1/api/config/save
```

Stop the motor:

```bash
curl -i -X POST http://192.168.4.1/api/motor/stop
```

Use `curl -i` to inspect both HTTP status and response body. If an endpoint returns an error, inspect the JSON error response and Serial logs.

### API troubleshooting

- **Connection refused / timeout:** verify the client is connected to the correct Wi-Fi network and that the ESP32 has booted the HTTP server.
- **404:** check the route and HTTP method.
- **400:** check JSON syntax, field names, value types and validation ranges.
- **409 on motor start:** a safety interlock may be active. Check sensor status and health; do not bypass it.
- **503 from health:** inspect the reported sensor fault and wiring. A responding HTTP server does not mean all hardware is healthy.
- **Unexpected old values after reboot:** persist the configuration and confirm that the save operation succeeded.
- **Credentials not visible in GET config:** this is intentional for secret fields. Do not expose passwords to make debugging easier.

### Security warning

The current project README states that the firmware API has no authentication. Treat the ESP32 network as trusted and isolated. Do not expose the device or its HTTP API directly to the public internet. The current CORS setting is not an authentication mechanism.

## 11. Frontend dashboard troubleshooting

The React dashboard is in `frontend/`.

Run locally:

```bash
cd frontend
npm install
npm run dev
```

Vite serves the dashboard on the development machine. Set the dashboard's device/API base URL to the ESP32 address (normally `http://192.168.4.1`) when using the ESP32 access point.

### Dashboard says disconnected

- Confirm the computer is connected to the ESP32 AP or the same LAN as the board.
- Open `http://192.168.4.1/api/health` directly in the browser.
- Check the configured API base URL in the dashboard.
- Check browser developer tools for network errors and CORS failures.
- Confirm the HTTP server has started and the firmware is running.

### Hosted Vercel dashboard cannot call the ESP32

The hosted dashboard is served over HTTPS, while the ESP32 API is plain HTTP. Browsers commonly block HTTPS pages from calling an HTTP device endpoint as mixed active content. This is a browser security restriction, not necessarily a firmware failure.

Use one of these approaches:
- Run the Vite dashboard locally over HTTP while on the same network as the ESP32.
- Serve the dashboard directly from the ESP32.
- Put a properly secured HTTPS proxy between the hosted dashboard and the device.

Do not ask users to disable browser security as a deployment solution.

### Dashboard loads but values are missing

- Inspect the actual JSON from `/api/status` and `/api/config`.
- Check the browser console for JavaScript errors.
- Confirm the dashboard and firmware agree on field names and response structure.
- Do not assume a missing field means a sensor is healthy; inspect the device health endpoint and logs.

## 12. Vercel deployment troubleshooting

The frontend Vercel project is configured with the `frontend/` directory as its project root and Vite as its framework.

Expected build settings:
- Root directory: `frontend`
- Install command: `npm install --no-audit --no-fund`
- Build command: `npm run build`
- Output directory: `dist`
- Node.js: `22.x`

From the frontend directory, the local production build command is:

```bash
npm install
npm run build
```

### Vercel still runs pnpm

Check the deployment's actual build logs rather than relying only on dashboard settings. Confirm that the project install command is set to npm and inspect repository lockfiles or package-manager metadata that may cause package-manager detection. The build log should explicitly show:

```text
npm install --no-audit --no-fund
```

If Vercel still invokes pnpm, inspect the project settings, root directory, lockfiles and deployment-specific configuration before retrying.

### Build fails

- Open the failed deployment's build logs.
- Confirm Vercel is using `frontend/` as the root directory.
- Check the exact install command and Node.js version.
- Reproduce with `npm run build` locally from `frontend/`.
- Fix the first meaningful TypeScript or Vite error; later errors may be consequences.
- Do not call the deployment successful until Vercel reports READY and the deployed page is checked.

### Deployment is protected by Vercel authentication

The project may have deployment protection enabled. A login prompt can be expected for protected deployment URLs. Check the Vercel project's protection settings and the correct production alias before treating a login screen as an application build failure.

## 13. Safe debugging sequence

Use this sequence to isolate issues without introducing additional failures:

1. Disconnect motor power and verify the ESP32 boots.
2. Open Serial Monitor at 115200 baud and capture startup logs.
3. Run `help`, `status` and `health`.
4. Verify AP visibility, connect to it and open the dashboard URL.
5. Call `/api/health`, `/api/status`, `/api/config` and `/api/logs` directly.
6. Test IR and ultrasonic sensors independently.
7. Verify LEDs and buzzer.
8. Verify motor driver wiring and external supply with the motor unloaded.
9. Test motor control only after confirming sensor readings and safety-stop behavior.
10. Test the dashboard and hosted deployment separately from firmware behavior.

Change one variable at a time and record the firmware commit, board package version, wiring changes and exact error output.

## 14. Known limitations and cautions

- Firmware behavior must be verified on the actual ESP32 hardware; documentation or a successful frontend build is not hardware validation.
- The API is currently documented as unauthenticated. Use only a trusted network.
- An HTTPS-hosted dashboard cannot normally call a plain HTTP ESP32 API directly from a browser due to mixed-content restrictions.
- The documented Wi-Fi configuration currently includes a single STA SSID/password pair. Do not assume 10-profile management or strongest-known-network selection exists until it is implemented and tested.
- The Arduino IDE firmware and root PlatformIO firmware are separate source locations. Keep them synchronized intentionally.
- Never connect a 5V sensor output directly to a non-5V-tolerant ESP32 GPIO.
- Never power a motor directly from the ESP32 3.3V supply.
- Never disable safety interlocks merely to make a test pass.
