# GPS NEO-7M Module - ESP32 Firmware

PlatformIO firmware for interfacing the u-blox NEO-7M GPS module with an ESP32 Dev Board.

## Hardware Requirements

- ESP32 Dev Board (ESP32-WROOM-32)
- NEO-7M GPS Module (u-blox)
- Jumper wires
- USB cable for programming

## NEO-7M Module Specifications

| Parameter | Value |
|-----------|-------|
| Chipset | u-blox NEO-7M |
| Input Voltage | 2.7V - 5.0V |
| Logic Level | 3.3V |
| Operating Current | 35-40mA |
| Default Baud Rate | 9600 bps |
| Update Rate | 1Hz (default), up to 10Hz |
| Cold Start Time | ~27 seconds |
| Hot Start Time | ~1 second |
| Supported GNSS | GPS |

## Wiring Diagram

```
NEO-7M GPS Module          ESP32 Dev Board
==================         ================
     VCC  -----------------> 5V (VIN pin)
     GND  -----------------> GND
     TX   -----------------> GPIO16 (RX2)
     RX   -----------------> GPIO17 (TX2)
     PPS  -----------------> GPIO4 (optional)
```

**IMPORTANT:** Use 5V for power, not 3.3V! The NEO-7M module has an onboard regulator and works more reliably with 5V input. The LED should light up when properly powered.

### Pin Connection Table

| NEO-7M Pin | ESP32 Pin | Description |
|------------|-----------|-------------|
| VCC | 5V (VIN) | Power supply (module has onboard 3.3V regulator) |
| GND | GND | Common ground |
| TX | GPIO16 | GPS transmits NMEA data to ESP32 (UART2 RX) |
| RX | GPIO17 | ESP32 sends commands to GPS (UART2 TX) |
| PPS | GPIO4 | Pulse-per-second output for precise timing (optional) |

### Important Notes

- **No level shifter required** when powering from 3.3V - the NEO-7M outputs 3.3V logic levels
- If powering from 5V, the module's onboard regulator converts to 3.3V internally
- GPIO16/17 are the default UART2 pins on ESP32 - safe to use, no boot conflicts
- The PPS pin outputs a 1Hz pulse synchronized to GPS time (useful for timing applications)

## Project Structure

```
gps-neo7m/
├── platformio.ini              # PlatformIO configuration
├── README.md                   # This file
├── include/
│   └── gps_config.h            # Pin definitions & configuration
├── lib/
│   └── GPSModule/
│       ├── GPSModule.h         # GPS library header
│       └── GPSModule.cpp       # GPS library implementation
└── src/
    └── main.cpp                # Main application
```

## Features

- **Location tracking** - Latitude, longitude with 6 decimal precision
- **Altitude** - Height above sea level in meters
- **Speed** - Ground speed in km/h and m/s
- **Course** - Heading/direction in degrees
- **Satellite info** - Number of satellites in view
- **HDOP** - Horizontal Dilution of Precision (accuracy indicator)
- **Date/Time** - UTC timestamp from GPS satellites
- **Fix quality** - 2D/3D fix status indication
- **Debug statistics** - Characters processed, checksum failures

## HDOP Quality Reference

| HDOP Value | Quality | Description |
|------------|---------|-------------|
| < 1 | Ideal | Highest accuracy |
| 1 - 2 | Excellent | Very accurate |
| 2 - 5 | Good | Accurate enough for most applications |
| 5 - 10 | Moderate | Acceptable for navigation |
| 10 - 20 | Fair | Low accuracy |
| > 20 | Poor | Very low accuracy |

## Building and Uploading

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (VS Code extension or CLI)

### Build

```bash
cd gps-neo7m
pio run
```

### Upload to ESP32

```bash
pio run -t upload
```

### Monitor Serial Output

```bash
pio device monitor
```

Or build, upload, and monitor in one command:

```bash
pio run -t upload && pio device monitor
```

## Serial Output Example

```
====================================
  GPS NEO-7M Module Test
  ESP32 Dev Board
====================================
Board: ESP32Dev

=== Wiring Guide ===
NEO-7M      ESP32
------      -----
VCC    -->  3.3V
GND    -->  GND
TX     -->  GPIO16 (RX2)
RX     -->  GPIO17 (TX2)
PPS    -->  GPIO4 (optional)

Initializing GPS module...
RX Pin: GPIO16
TX Pin: GPIO17
Baud Rate: 9600
GPS module initialized successfully!

====================================
  Waiting for GPS Fix...
  (Place module outdoors or near window)
  Cold start may take 1-2 minutes
====================================

========== GPS Data ==========
Fix Status: 3D Fix (8 satellites)
Location:   12.345678, 98.765432
Altitude:   45.2 m
Speed:      0.5 km/h (0.1 m/s)
Course:     125.3 deg
HDOP:       1.20 (Excellent)
Date/Time:  2026-01-21 15:30:45 UTC
Fix Age:    125 ms
==============================
```

## Troubleshooting

### No data received from GPS

1. **Check wiring** - Ensure TX/RX are not swapped (GPS TX -> ESP32 RX)
2. **Verify power** - Module needs stable 3.3V or 5V
3. **Check baud rate** - Default is 9600, ensure it matches configuration

### High checksum failures

1. **Check connections** - Loose wires can cause data corruption
2. **Reduce interference** - Keep GPS away from noise sources
3. **Verify baud rate** - Mismatched baud rate causes parsing errors

### No GPS fix (location invalid)

1. **Move outdoors** - GPS requires clear sky view
2. **Wait for cold start** - First fix can take 1-2 minutes
3. **Check antenna** - Ensure the ceramic antenna is facing up
4. **Avoid obstructions** - Buildings, trees, and metal objects block signals

### Fix but low accuracy (high HDOP)

1. **Improve sky view** - More visible sky = more satellites
2. **Wait longer** - Accuracy improves as more satellites are acquired
3. **Check for multipath** - Reflections from buildings degrade accuracy

## Configuration

Pin assignments can be changed in `platformio.ini`:

```ini
build_flags =
    -D GPS_RX_PIN=16      # Change ESP32 RX pin
    -D GPS_TX_PIN=17      # Change ESP32 TX pin
    -D GPS_PPS_PIN=4      # Change PPS pin (optional)
    -D GPS_BAUD=9600      # Change baud rate if needed
```

## Dependencies

- [TinyGPS++](https://github.com/mikalhart/TinyGPSPlus) - NMEA parsing library (installed automatically)

## License

MIT License - Feel free to use and modify for your projects.
