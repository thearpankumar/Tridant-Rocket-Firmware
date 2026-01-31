# HX711 Load Cell Module - ESP32 Thrust Test System

High-speed (80Hz) thrust measurement system for rocket motor testing with real-time web dashboard.

## Features

- **80Hz Sampling Rate** - High-speed data acquisition for accurate thrust curves
- **Real-time Web Dashboard** - Beautiful dark-themed visualization accessible via WiFi
- **Live Metrics** - Peak thrust, total impulse, burn time, average thrust
- **CSV Export** - Download test data directly from browser
- **Offline Operation** - Works in AP mode without internet connection

## Hardware Requirements

- ESP32 Dev Board (ESP32-WROOM-32)
- HX711 Load Cell Amplifier Module (RATE pin set to HIGH for 80Hz)
- Load Cell (strain gauge type, 4-wire)
- Jumper wires

## Pin Configuration

### HX711 to ESP32 Wiring

| HX711 Pin | ESP32 Pin | Description |
|-----------|-----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| DT | GPIO16 | Data output |
| SCK | GPIO4 | Serial clock |

### Load Cell to HX711 Wiring

| Load Cell Wire | HX711 Pin | Description |
|----------------|-----------|-------------|
| Red | E+ | Excitation positive |
| Black | E- | Excitation negative |
| White | A- | Signal negative |
| Green | A+ | Signal positive |

> **Note:** Wire colors may vary between manufacturers. If readings are negative, try swapping A+ and A- connections.

### 80Hz Mode Setup

**IMPORTANT:** For 80Hz operation, modify your HX711 board:
- Locate the RATE pin jumper on the HX711 module
- Either: cut trace to GND, or bridge RATE to VCC
- This enables 80Hz instead of default 10Hz

## Quick Start

### 1. Build and Upload

```bash
# Build firmware
pio run

# Upload firmware to ESP32
pio run -t upload

# Upload web dashboard files
pio run -t uploadfs
```

### 2. Connect to Dashboard

1. Power on the ESP32
2. Connect to WiFi network: **ThrustTest** (password: `rocket123`)
3. Open browser and navigate to: **http://192.168.4.1**

### 3. Run a Thrust Test

1. Click **TARE** to zero the sensor (ensure no load)
2. Click **START** to begin recording
3. Fire your motor
4. Click **STOP** when test complete
5. Click **EXPORT CSV** to download data

## Web Dashboard

### Live Metrics

| Metric | Description | Unit |
|--------|-------------|------|
| Current Thrust | Real-time reading | N |
| Peak Thrust | Maximum achieved | N |
| Total Impulse | Area under thrust curve | Ns |
| Burn Time | Duration above 5% peak | s |
| Average Thrust | Mean during burn | N |
| Samples | Data points collected | count |

### Controls

| Button | Function |
|--------|----------|
| TARE | Zero the sensor |
| START/STOP | Begin/end recording session |
| RESET | Clear all data and metrics |
| EXPORT CSV | Download test data |
| CALIBRATE | Calibrate with known weight |

### Dashboard Features

- Real-time thrust curve with ApexCharts
- Peak marker annotation on chart
- 80Hz data streaming via WebSocket
- Mobile-responsive dark theme
- Works offline (AP mode)

## Serial Commands

Commands available via serial monitor (921600 baud):

| Command | Description |
|---------|-------------|
| `t` / `T` | Tare (zero) the sensor |
| `r` / `R` | Show raw ADC reading |
| `p` / `P` | Pause/resume output |
| `z` / `Z` | Reset timestamp to 0 |
| `c` / `C` | Enter calibration mode |
| `h` / `H` | Show help |

## Calibration

### Via Serial (Recommended)

1. Open serial monitor at 921600 baud
2. Press `c` to enter calibration mode
3. Follow prompts: remove weight → press Enter → add known weight → enter weight in grams
4. Note the calibration factor
5. Update `platformio.ini`:
   ```ini
   -D CALIBRATION_FACTOR=YOUR_VALUE
   ```

### Via Web Dashboard

1. Enter known weight in grams in the calibration input
2. Click CALIBRATE
3. Follow serial output for new calibration factor

## Project Structure

```
loadcell-hx711/
├── platformio.ini              # PlatformIO configuration
├── README.md                   # This file
├── include/
│   ├── loadcell_config.h       # Load cell configuration
│   └── wifi_config.h           # WiFi/dashboard configuration
├── lib/
│   ├── LoadCellModule/         # Load cell driver
│   │   ├── LoadCellModule.h
│   │   └── LoadCellModule.cpp
│   └── WebDashboard/           # Web dashboard module
│       ├── WebDashboard.h
│       ├── WebDashboard.cpp
│       └── ThrustMetrics.h
├── data/                       # Web assets (LittleFS)
│   ├── index.html
│   ├── css/
│   │   └── dashboard.css
│   └── js/
│       ├── apexcharts.js       # Charting library
│       ├── app.js              # Main application
│       ├── chart.js            # Chart configuration
│       ├── websocket.js        # WebSocket handler
│       └── metrics.js          # Metrics display
└── src/
    └── main.cpp                # Main application
```

## Configuration

### platformio.ini Build Flags

| Flag | Default | Description |
|------|---------|-------------|
| `CALIBRATION_FACTOR` | 1500.0 | Load cell calibration factor |
| `LOADCELL_DOUT_PIN` | 16 | HX711 data pin |
| `LOADCELL_SCK_PIN` | 4 | HX711 clock pin |
| `ENABLE_WEB_DASHBOARD` | defined | Enable/disable web dashboard |

### WiFi Configuration (wifi_config.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `WIFI_AP_SSID` | "ThrustTest" | WiFi network name |
| `WIFI_AP_PASSWORD` | "rocket123" | WiFi password |
| `WS_DATA_RATE_HZ` | 80 | WebSocket data rate |
| `WS_METRICS_RATE_HZ` | 4 | Metrics update rate |

### Disabling Web Dashboard

To disable the web dashboard and reduce firmware size:

1. Comment out in `platformio.ini`:
   ```ini
   ; -D ENABLE_WEB_DASHBOARD
   ```
2. Rebuild: `pio run`

## WebSocket Protocol

### ESP32 → Browser (80Hz data)
```json
{"type":"data","t":12345,"f":125.430}
```

### ESP32 → Browser (4Hz metrics)
```json
{"type":"metrics","peak":342.5,"impulse":128.7,"burn":2.45,"avg":52.3,"samples":196,"recording":true}
```

### Browser → ESP32 (commands)
```json
{"cmd":"tare"}
{"cmd":"start"}
{"cmd":"stop"}
{"cmd":"reset"}
{"cmd":"calibrate","value":500}
```

## Dependencies

- [bogde/HX711](https://github.com/bogde/HX711) - HX711 driver
- [mathieucarbou/ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) - Async web server
- [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) - JSON parsing
- [ApexCharts](https://apexcharts.com/) - Real-time charting (bundled)

## Troubleshooting

### Dashboard not loading
- Ensure you uploaded filesystem: `pio run -t uploadfs`
- Check you're connected to "ThrustTest" WiFi
- Try http://192.168.4.1 (not https)

### No data on chart
- Verify HX711 wiring
- Check serial output for errors
- Ensure RATE pin is HIGH for 80Hz

### Unstable readings
- Check load cell mounting
- Verify wiring connections
- Allow warmup time before testing

### WebSocket disconnects
- Move closer to ESP32
- Check for WiFi interference
- Reduce number of connected clients

## License

This project is part of the Tridant Rocket Firmware test suite.
