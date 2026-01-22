# HX711 Load Cell Module - ESP32

This project provides firmware for reading weight measurements from a load cell using the HX711 24-bit ADC amplifier with an ESP32 development board.

## Hardware Requirements

- ESP32 Dev Board (ESP32-WROOM-32)
- HX711 Load Cell Amplifier Module
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

> **Note:** Wire colors may vary between manufacturers. Common alternatives:
> - Red/Black for excitation (power)
> - White/Green or Blue/Yellow for signal
>
> If readings are negative, try swapping A+ and A- connections.

## Wiring Diagram

```
                    +-----------+
                    |   ESP32   |
                    |           |
    +-------+       |   3.3V ---+--- VCC
    | Load  |       |    GND ---+--- GND     +-------+
    | Cell  |------>|  GPIO16 --+--- DT      | HX711 |
    |       |       |   GPIO4 --+--- SCK     +-------+
    +-------+       |           |                |
        |           +-----------+                |
        |                                        |
        +---- Red (E+) --------------------------+
        +---- Black (E-) ------------------------+
        +---- White (A-) ------------------------+
        +---- Green (A+) ------------------------+
```

## Software Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB driver for ESP32 (CP2102 or CH340 depending on your board)

### Building and Uploading

```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

Or use the combined command:
```bash
pio run -t upload && pio device monitor
```

## Calibration

Before using the scale for accurate measurements, you must calibrate it with a known weight.

### Step 1: Enable Calibration Mode

Edit `src/main.cpp` and change:
```cpp
#define CALIBRATION_MODE false
```
to:
```cpp
#define CALIBRATION_MODE true
```

Also set your known calibration weight:
```cpp
#define KNOWN_WEIGHT 100.0  // Change to your known weight in grams
```

### Step 2: Run Calibration

1. Upload the firmware and open the serial monitor
2. Remove all weight from the load cell and press ENTER
3. Place your known weight on the scale and press ENTER
4. Note the calibration factor displayed

### Step 3: Apply Calibration Factor

Update the calibration factor in `include/loadcell_config.h`:
```cpp
#define CALIBRATION_FACTOR -471.497  // Replace with your value
```

Or add it to `platformio.ini` build flags:
```ini
build_flags =
    -D CALIBRATION_FACTOR=-471.497
```

### Step 4: Disable Calibration Mode

Change back to normal mode:
```cpp
#define CALIBRATION_MODE false
```

Rebuild and upload.

## Serial Commands

During normal operation, you can send these commands via serial monitor:

| Command | Description |
|---------|-------------|
| `t` or `T` | Tare (zero) the scale |
| `r` or `R` | Show raw ADC reading |
| `c` or `C` | Show calibration factor |
| `h` or `H` | Show help menu |

## Project Structure

```
loadcell-hx711/
├── platformio.ini              # PlatformIO configuration
├── README.md                   # This file
├── include/
│   └── loadcell_config.h       # Configuration and constants
├── lib/
│   └── LoadCellModule/         # Custom load cell library
│       ├── LoadCellModule.h
│       └── LoadCellModule.cpp
└── src/
    └── main.cpp                # Main application
```

## Configuration Options

Edit `include/loadcell_config.h` to customize:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `CALIBRATION_FACTOR` | -471.497 | Scale calibration factor |
| `READINGS_TO_AVERAGE` | 10 | Number of readings for averaging |
| `DISPLAY_INTERVAL` | 1000 | Weight display interval (ms) |
| `STABILITY_THRESHOLD` | 0.5 | Stability detection threshold (g) |
| `STABILITY_SAMPLES` | 5 | Samples for stability check |
| `MIN_WEIGHT_THRESHOLD` | 0.5 | Noise filter threshold (g) |
| `TARE_READINGS` | 20 | Readings for tare operation |

## HX711 Specifications

| Parameter | Value |
|-----------|-------|
| Input Voltage | 2.6V - 5.5V |
| Operating Current | < 1.5mA |
| Power Down Current | < 1uA |
| Data Rate | 10Hz or 80Hz |
| Resolution | 24-bit ADC |
| Gain (Channel A) | 128 or 64 |
| Gain (Channel B) | 32 |

## Troubleshooting

### No readings / HX711 not responding
- Check power connections (VCC and GND)
- Verify DT and SCK pins are connected correctly
- Ensure load cell is properly wired to HX711

### Negative readings
- Swap A+ and A- connections on the HX711
- Or change the sign of your calibration factor

### Unstable/noisy readings
- Increase `READINGS_TO_AVERAGE` for more smoothing
- Check for loose connections
- Ensure load cell is mounted securely
- Keep wires away from interference sources

### Readings drift over time
- Allow the HX711 to warm up for a few minutes
- Re-tare the scale periodically
- Check for temperature changes affecting the load cell

## Dependencies

- [bogde/HX711](https://github.com/bogde/HX711) - Arduino library for HX711

## License

This project is part of the Tridant Rocket Firmware test suite.
