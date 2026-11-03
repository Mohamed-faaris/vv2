# ESP32 Wi-Fi Board Development Guidelines

## 1. Purpose and scope

This document defines reusable engineering requirements for ESP32 firmware. The goal is firmware that is modular, configurable, observable, testable and recoverable without repeatedly editing source code or reflashing the board.

These requirements are hardware-independent. Each project must document its own board model, GPIO assignments, connected components, electrical limits and application-specific behavior separately.

The firmware must be manageable through two first-class interfaces:

- **Serial Command-Line Interface (CLI):** direct development, bench testing and troubleshooting.
- **HTTP API:** remote configuration, testing, diagnostics and integration with dashboards or other applications.

Both interfaces must call the same internal application and service functions. Do not duplicate business logic between CLI handlers and HTTP handlers.

## 2. Core engineering principles

1. Keep firmware modular. Separate hardware access, application logic, configuration, networking, diagnostics, logging, CLI and API code.
2. Centralize hardware definitions and configurable values. Avoid scattered pin numbers, magic numbers and duplicated defaults.
3. Use meaningful names, focused functions, consistent formatting and comments for non-obvious decisions.
4. Validate all external inputs, including serial commands, API payloads, configuration values and sensor readings.
5. Prefer non-blocking state machines, bounded operations and event-driven callbacks. Network activity must not unnecessarily stop sensor sampling or safety logic.
6. Make important runtime settings configurable where practical.
7. Persist user configuration across restarts and provide explicit defaults and reset behavior.
8. Preserve working functionality when adding features. Avoid unrelated rewrites and undocumented breaking changes.
9. Define safe startup and failure states for all outputs. Safety-critical behavior must not depend on the dashboard or network being connected.
10. Distinguish clearly between implemented, compiled, bench-tested and hardware-validated functionality.

## 3. Modular firmware architecture

Organize the firmware into focused modules. Names may vary, but responsibilities should remain clear.

| Module | Responsibility |
|---|---|
| Hardware abstraction | Board definitions, GPIO assignments, initialization and safe output defaults |
| Sensors | Sampling, conversion, validation, calibration and fault reporting |
| Outputs / actuators | Controlled access to motors, LEDs, relays and other outputs |
| Wi-Fi manager | AP, STA, AP+STA, saved profiles, scanning and reconnection |
| Configuration | Runtime settings, validation, defaults and persistent storage |
| CLI | Serial command parsing, help and command responses |
| HTTP API | Request parsing, authentication, response formatting and routing |
| Logger | Consistent logs, levels and optional log retention |
| Diagnostics | Health checks, subsystem tests, fault summaries and exports |
| Application controller | Initialization, state transitions and application coordination |

Keep hardware-specific code behind clear interfaces where practical. Application logic should not need to know low-level driver details.

## 4. CLI and HTTP API are first-class interfaces

Every important operational capability should be available through both Serial CLI and HTTP API wherever the operation is safe and meaningful.

### 4.1 Required shared capabilities

| Capability | Serial CLI | HTTP API |
|---|---:|---:|
| Device and firmware information | Yes | Yes |
| Overall status and uptime | Yes | Yes |
| System and subsystem health | Yes | Yes |
| Sensor readings and sensor tests | Yes | Yes |
| Output / actuator tests | Yes | Yes |
| Wi-Fi mode, status and connection state | Yes | Yes |
| Wi-Fi scan | Yes | Yes |
| Saved Wi-Fi profile management | Yes | Yes |
| Connect to a selected network | Yes | Yes |
| Connect to strongest known network | Yes | Yes |
| Configuration view and modification | Yes | Yes |
| Save, load and reset configuration | Yes | Yes |
| Logs, errors and diagnostic summaries | Yes | Yes |
| Restart, where authorized | Yes | Yes |

CLI and API must use the same validation, state transitions and underlying service methods. If an operation is unsupported through one interface, document why rather than silently implementing different behavior.

### 4.2 CLI requirements

- Provide a discoverable `help` command with syntax and examples.
- Use logical command groups such as `status`, `health`, `wifi`, `sensor`, `test`, `config` and `log`.
- Report whether a command was accepted, rejected, started, completed or timed out.
- Validate argument count, types, ranges and missing values.
- Handle unknown commands and malformed input without crashing or corrupting configuration.
- Never echo Wi-Fi passwords or other secrets.
- Make long-running operations asynchronous or provide a bounded wait/status mechanism. Do not block critical application tasks while scanning or connecting.

### 4.3 HTTP API requirements

- Use consistent resource names, HTTP methods and response schemas.
- Use GET for reading state and POST/PATCH/DELETE for operations or changes as appropriate.
- Validate request bodies, content types, field lengths and allowed values.
- Return meaningful HTTP status codes and structured error details.
- Distinguish a request being accepted from the requested operation actually succeeding. For asynchronous operations, return an operation identifier or state that the client can poll.
- Never put passwords, tokens or secrets in URL paths or query parameters. Send credentials in protected request bodies.
- Apply authentication and authorization to configuration changes, network credentials, actuator tests and restart operations.
- Set reasonable request-size limits and timeouts.

A representative response is:

```json
{
  "ok": true,
  "state": "in_progress",
  "operation_id": "example-operation-id",
  "message": "Connection attempt started"
}
```

Error responses should include a stable error code and a human-readable message. Do not expose secrets or internal memory contents.

## 5. Testing, debugging and diagnostics

Testing and debugging through both CLI and API are mandatory design goals.

### 5.1 Health checks

Provide an overall health report and subsystem-level health checks. Include, where applicable:

- Firmware version, build information and uptime.
- Reset reason and restart count if available.
- Main application state.
- Sensor availability and last valid reading time.
- Sensor faults, out-of-range readings and communication failures.
- Output / actuator state and detected faults.
- Wi-Fi mode, connection state, signal strength and IP addresses.
- Configuration validity and persistent-storage status.
- Available memory and other relevant resource indicators.
- Recent errors and active warnings.

A health check must report actual subsystem state. It must not return success only because the HTTP server responded.

### 5.2 Safe component tests

Support targeted tests for individual sensors, outputs and communication subsystems. Each test should state what it checks, its result and any failure reason.

- Respect electrical and mechanical limits.
- Apply duration or magnitude limits to actuator tests where appropriate.
- Provide a reliable stop or abort operation for tests that can move hardware.
- Do not allow diagnostic commands to bypass application safety interlocks.
- Avoid concurrent tests that conflict over the same hardware resource.
- Report unsupported tests explicitly instead of returning a misleading pass.

### 5.3 Logs and fault reporting

- Provide log levels such as ERROR, WARN, INFO, DEBUG and TRACE.
- Include subsystem and useful context in each message.
- Prevent high-frequency logs from overwhelming Serial output or storage.
- Record actionable fault causes and recovery state changes.
- Never log Wi-Fi passwords, API credentials, private keys or other secrets.
- Make recent logs or a bounded diagnostic summary available through CLI and API.

### 5.4 Diagnostics export

Provide a diagnostic report for support and debugging. It may include firmware/build version, uptime, non-sensitive board information, configuration with secrets removed, Wi-Fi state without passwords, subsystem health, recent errors and test results.

Diagnostic exports must redact credentials, tokens, private keys and other sensitive values.

## 6. Configuration and persistent storage

- Centralize defaults and validation rules.
- Separate current runtime configuration from persisted configuration.
- Support reading, validating, applying, saving, loading and resetting configuration.
- Make it clear whether a change is temporary or has been saved permanently.
- Avoid partially applying invalid configuration. Validate a candidate configuration before committing it.
- Handle missing, corrupted or older stored configuration safely. Fall back to documented defaults and report the issue.
- Use ESP32 NVS / the Arduino `Preferences` API or another suitable persistent store for small settings and credentials.
- Version stored configuration where schema changes are likely.

## 7. Wi-Fi operating modes

The Wi-Fi manager must support the ESP32's available operating modes:

### 7.1 Access Point (AP)

The ESP32 creates its own Wi-Fi network. A phone or computer can connect directly to the board without an external router.

Requirements:
- Configurable AP SSID and password.
- Configurable channel and client limit where supported.
- Report AP state, IP address and connected-client count where available.
- Allow the user to change AP credentials through authorized CLI and API operations.
- Validate SSID and password lengths and document open-network behavior if supported.

### 7.2 Station (STA)

The ESP32 connects to an existing Wi-Fi router.

Requirements:
- Allow explicit SSID/password configuration.
- Report connection state, current SSID, RSSI, IP address, gateway and DNS where available.
- Distinguish disconnected, associating, authentication failure, DHCP failure and connected states where the platform exposes enough information.
- Support disconnect and reconnect operations.
- Use bounded retries and backoff rather than a tight reconnect loop.

### 7.3 Combined AP + STA

Support AP+STA operation so the device can maintain its own setup / recovery network while joining an external router, subject to the ESP32 model and framework limitations.

The AP should remain available during STA scanning and connection attempts whenever the selected hardware mode supports it. A failed router connection must not permanently remove the user's local recovery path.

Document radio/channel limitations of the selected ESP32 and framework. AP+STA shares one radio; channel behavior can be affected by the upstream access point.

### 7.4 Wi-Fi mode selection

Provide explicit configuration for AP, STA, AP+STA and Wi-Fi-off modes. Validate that at least one interface is enabled when network access is required by the application.

## 8. Store up to 10 Wi-Fi profiles

The firmware must persist up to **10 Wi-Fi network profiles**.

Each profile should contain:
- SSID.
- Password.
- Optional user-friendly profile name.
- Optional priority.
- Last successful connection time or outcome, where practical.
- Optional failure/backoff metadata.

Required profile operations:

1. Add a profile.
2. Update an existing profile's credentials or metadata.
3. Remove a profile.
4. List profiles without revealing passwords.
5. Retrieve profile metadata by index or stable profile ID.
6. Check capacity and report when all 10 slots are occupied.
7. Save profiles persistently and restore them after restart.
8. Clear profiles only through an explicit authorized action.

Define whether duplicate SSIDs replace an existing profile or are rejected. Validate SSID and password lengths before storage. Avoid writing to flash unnecessarily; update persistent storage only when a profile actually changes.

Never include plaintext passwords in profile-list responses, logs, CLI output or diagnostic exports.

## 9. Automatic connection to the strongest known Wi-Fi

Provide an automatic mode that connects to the strongest currently available network among the saved profiles.

Recommended process:

1. Scan for nearby Wi-Fi networks.
2. Match discovered SSIDs against saved profiles.
3. Ignore saved profiles that are not currently visible.
4. Compare signal strength (RSSI) among eligible networks.
5. Select a candidate using signal strength, priority and recent failure history.
6. Attempt connection with a timeout.
7. Verify that the ESP32 actually connects and receives network configuration.
8. On failure, record the reason and try the next eligible profile, subject to retry limits.
9. Report the selected profile and final outcome without exposing credentials.

Do not repeatedly switch networks because of small RSSI fluctuations. Use a minimum signal improvement, stability period or hysteresis. Include a manual override so the user can keep a selected network instead of automatic selection.

## 10. Explicit connection requests

Automatic selection must not be the only connection method. The user must be able to request a specific network.

Support:
- Connect to a saved profile by profile ID, name or index.
- Connect to a specific SSID using credentials provided in the request.
- Save a newly supplied network profile, if requested.
- Connect once without saving credentials, if supported.
- Reconnect to the current or selected saved profile.
- Disconnect from the current STA network.
- Cancel a pending connection attempt.
- Return the result and a useful failure reason.

An explicit connection request should take precedence over automatic selection until it succeeds, fails, times out or is cancelled. Do not silently replace the user's requested network with a different profile.

## 11. Wi-Fi operations through CLI and API

Provide equivalent operations through both interfaces.

| Operation | CLI example | API intent |
|---|---|---|
| Status | `wifi status` | Read current Wi-Fi state |
| Scan | `wifi scan` | Start scan and retrieve results |
| List saved profiles | `wifi list` | List profile metadata, never passwords |
| Add profile | `wifi add <ssid> <password>` | Add profile using protected request body |
| Update profile | `wifi update <id> <ssid> <password>` | Update profile using protected request body |
| Remove profile | `wifi remove <id>` | Delete a saved profile |
| Connect to profile | `wifi connect <id>` | Connect to a selected saved profile |
| Connect once | `wifi connect-now <ssid> <password>` | Connect using request credentials |
| Auto-select | `wifi auto` | Connect to strongest known network |
| Disconnect | `wifi disconnect` | Disconnect STA |
| Cancel attempt | `wifi cancel` | Cancel the current operation |
| Set AP credentials | `wifi ap set <ssid> <password>` | Update AP configuration |
| Set mode | `wifi mode ap|sta|apsta|off` | Change Wi-Fi mode |

CLI parsers must handle spaces and special characters in SSIDs/passwords safely. Prefer quoted arguments or a structured input prompt instead of fragile whitespace splitting. Never print the supplied password after parsing it.

Suggested HTTP resource design (adapt to the project's existing API conventions):

- `GET /api/wifi/status`
- `GET /api/wifi/scan` or `POST /api/wifi/scan` plus an operation-status route
- `GET /api/wifi/profiles`
- `POST /api/wifi/profiles`
- `PATCH /api/wifi/profiles/{id}`
- `DELETE /api/wifi/profiles/{id}`
- `POST /api/wifi/connect`
- `POST /api/wifi/connect/auto`
- `POST /api/wifi/disconnect`
- `POST /api/wifi/cancel`
- `PATCH /api/wifi/mode`
- `PATCH /api/wifi/ap`

These are design suggestions, not a requirement to break an existing API. Keep API routes documented and version them if incompatible changes are introduced.

## 12. Credential and network security

Wi-Fi passwords are secrets.

- Never print passwords in logs, Serial output, profile listings, status responses or diagnostic exports.
- Never accept passwords in URL query strings.
- Require authentication for sensitive network and configuration operations.
- Use HTTPS or another protected transport when credentials cross an untrusted network. Plain HTTP on an isolated provisioning AP may be an explicit provisioning trade-off, but must be documented.
- Restrict CORS to trusted dashboard origins where possible. CORS is not authentication.
- Use strong, configurable AP credentials. Do not rely on default passwords for deployment.
- Store credentials in NVS and protect them with the strongest practical device-level controls. NVS storage alone should not be described as protection against physical flash extraction; consider flash encryption and secure boot for deployments requiring stronger protection.
- Do not expose saved credentials through unauthenticated endpoints.
- Provide a deliberate credential erase/reset procedure.

## 13. Reliability and recovery

Handle these conditions without hanging or making the device permanently unreachable:

- Router unavailable or out of range.
- Incorrect password or authentication rejection.
- Weak or fluctuating signal.
- Scan failure or timeout.
- DHCP failure.
- STA disconnection after a successful connection.
- AP client disconnection.
- ESP32 reboot during an operation.
- Empty or corrupted saved-profile storage.
- All 10 profile slots occupied.
- Repeated connection failures.

Use retry limits, exponential or bounded backoff, connection timeouts and clear state reporting. Do not block safety or application control while waiting for a network. Preserve AP recovery access when possible. Store enough state to recover cleanly after reboot, but do not persist a transient "connecting" state as if it were a successful connection.

## 14. Acceptance checklist

A Wi-Fi implementation should not be considered complete until these checks pass:

- [ ] AP mode can be enabled, configured and inspected.
- [ ] STA mode can connect to an explicitly selected network.
- [ ] AP+STA operation works on the target ESP32 and framework.
- [ ] AP remains available during STA failure/recovery when supported.
- [ ] Ten profiles can be added and retained across reboot.
- [ ] An eleventh profile is rejected cleanly without corrupting existing profiles.
- [ ] Profiles can be listed, updated and deleted without revealing passwords.
- [ ] Automatic selection chooses an available saved network using documented selection rules.
- [ ] A user can explicitly select a saved profile.
- [ ] A user can connect once using a supplied SSID/password.
- [ ] CLI and API produce consistent results for equivalent operations.
- [ ] Scan/connect operations are bounded and do not block critical application logic.
- [ ] Authentication failures, DHCP failures and disconnections are reported accurately.
- [ ] Credentials do not appear in logs, API responses or diagnostic exports.
- [ ] The board remains recoverable after an invalid credential or unreachable router.
- [ ] Behavior has been verified on the actual target board, not only assumed from compilation.

## 15. Definition of done

An ESP32 board should be operable without repeatedly editing source code. A developer or user should be able to inspect the device, test hardware, configure settings, manage Wi-Fi, connect to a chosen or strongest known network, diagnose failures and recover using either Serial CLI or HTTP API.

Do not claim a feature is implemented or tested until its code, build and target-board behavior have been verified.
