# MAX31865 RTD Temperature Sensor - ESP32

This project implements temperature reading using the MAX31865 RTD-to-Digital converter with an ESP32 development board.

> **Important Note:** The MAX31865 is an **RTD (Resistance Temperature Detector)** amplifier designed for PT100/PT1000 sensors. It is **NOT** a thermocouple amplifier. For thermocouples, use MAX31855 or MAX31856.

## Hardware Requirements

- ESP32 Development Board (ESP32-WROOM-32)
- MAX31865 Breakout Board (Adafruit or compatible)
- PT100 or PT1000 RTD Sensor
- Jumper wires

## Pin Connections

### MAX31865 to ESP32 (VSPI)

| MAX31865 Pin | ESP32 Pin | Description |
|--------------|-----------|-------------|
| VIN | 3.3V | Power supply (3.0V - 3.6V) |
| GND | GND | Ground |
| CLK | GPIO18 | SPI Clock (VSPI CLK) |
| SDO | GPIO19 | SPI Data Out / MISO (VSPI MISO) |
| SDI | GPIO23 | SPI Data In / MOSI (VSPI MOSI) |
| CS | GPIO5 | Chip Select (VSPI SS) |

### RTD Sensor to MAX31865

The MAX31865 supports 2-wire, 3-wire, and 4-wire RTD configurations.

#### 3-Wire Configuration (Default - Most Common)

| RTD Wire | MAX31865 Terminal |
|----------|-------------------|
| Wire 1 | RTD+ |
| Wire 2 | RTD- |
| Wire 3 | F- |

> **Note:** On Adafruit boards, you need to solder the jumpers for 3-wire mode. Check your board's documentation.

#### 2-Wire Configuration

| RTD Wire | MAX31865 Terminal |
|----------|-------------------|
| Wire 1 | RTD+ and F+ (jumpered) |
| Wire 2 | RTD- and F- (jumpered) |

#### 4-Wire Configuration (Highest Accuracy)

| RTD Wire | MAX31865 Terminal |
|----------|-------------------|
| Wire 1 | F+ |
| Wire 2 | RTD+ |
| Wire 3 | RTD- |
| Wire 4 | F- |

## Wiring Diagram

```
                    ESP32 Dev Board
                   ┌───────────────┐
                   │               │
    MAX31865       │     3.3V  ────┼──── VIN
   ┌─────────┐     │      GND  ────┼──── GND
   │         │     │   GPIO18  ────┼──── CLK
   │  PT100  │     │   GPIO19  ────┼──── SDO (MISO)
   │   RTD   │     │   GPIO23  ────┼──── SDI (MOSI)
   │         │     │    GPIO5  ────┼──── CS
   └─────────┘     │               │
                   └───────────────┘
```

## RTD Sensor Types

| Sensor Type | Resistance at 0°C | Reference Resistor | Config Define |
|-------------|-------------------|-------------------|---------------|
| PT100 | 100 ohms | 430 ohms | `RTD_TYPE=100` |
| PT1000 | 1000 ohms | 4300 ohms | `RTD_TYPE=1000` |

## Configuration

Edit `platformio.ini` to change pin assignments or RTD type:

```ini
build_flags =
    ${env.build_flags}
    -D BOARD_ESP32_DEV
    -D MAX31865_CS_PIN=5
    -D MAX31865_MOSI_PIN=23
    -D MAX31865_MISO_PIN=19
    -D MAX31865_CLK_PIN=18
    -D BOARD_NAME=\"ESP32Dev\"
    ; Uncomment for PT1000:
    ; -D RTD_TYPE=1000
    ; Uncomment for 2-wire or 4-wire:
    ; -D RTD_WIRES=2
    ; -D RTD_WIRES=4
```

## Building and Uploading

```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor
```

## Serial Commands

| Command | Description |
|---------|-------------|
| `r` | Show raw ADC value and resistance |
| `a` | Show averaged temperature (10 readings) |
| `f` | Show fault status |
| `c` | Clear fault |
| `s` | Show sensor status |
| `h` | Show help menu |

## Serial Output Example

```
====================================
  MAX31865 RTD Temperature Sensor Test
  ESP32 Dev Board
====================================
Board: ESP32Dev

=== Wiring Guide ===
MAX31865     ESP32 (VSPI)
--------     -----------
VIN     -->  3.3V
GND     -->  GND
CLK     -->  GPIO18
SDO     -->  GPIO19
SDI     -->  GPIO23
CS      -->  GPIO5

Initializing MAX31865...
RTD Type: PT100
Reference Resistor: 430.00 ohms
Wire Configuration: 3-wire

MAX31865 initialized successfully!

Temp: 24.56 C (76.21 F) [STABLE]
```

## Specifications

### MAX31865

- **ADC Resolution:** 15-bit
- **Temperature Resolution:** ~0.03125°C
- **Conversion Time:** 21ms typical
- **Interface:** SPI (up to 5MHz)
- **Operating Voltage:** 3.0V - 3.6V

### PT100 Sensor

- **Temperature Range:** -200°C to +850°C
- **Accuracy:** Class A: ±(0.15 + 0.002×|t|)°C
- **Nominal Resistance:** 100 ohms at 0°C

## Fault Detection

The MAX31865 can detect the following fault conditions:

| Fault | Description |
|-------|-------------|
| RTD High Threshold | RTD resistance too high |
| RTD Low Threshold | RTD resistance too low |
| REFIN- > 0.85 × VBIAS | Reference input issue |
| REFIN- < 0.85 × VBIAS | FORCE- open |
| RTDIN- < 0.85 × VBIAS | FORCE- open |
| Overvoltage/Undervoltage | Power supply issue |

## Troubleshooting

### No temperature reading / Fault detected

1. Check all wiring connections
2. Verify the RTD sensor is connected properly
3. Check solder jumpers on the MAX31865 board match your wire configuration
4. Ensure 3.3V power supply is stable

### Incorrect temperature readings

1. Verify `RTD_TYPE` matches your sensor (PT100 = 100, PT1000 = 1000)
2. Check `RTD_RREF` matches your board's reference resistor
3. Verify wire configuration matches physical setup

### SPI communication failure

1. Check CLK, SDO, SDI, CS connections
2. Verify no loose connections
3. Try different GPIO pins if available

## File Structure

```
thermocoupler-max31865/
├── platformio.ini          # PlatformIO configuration
├── README.md               # This file
├── include/
│   └── rtd_config.h        # Configuration header
├── lib/
│   └── RTDModule/
│       ├── RTDModule.h     # RTD module class header
│       └── RTDModule.cpp   # RTD module implementation
└── src/
    └── main.cpp            # Main application
```

## Dependencies

- [Adafruit MAX31865 library](https://github.com/adafruit/Adafruit_MAX31865) (v1.6.2+)
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) (auto-installed)

## License

MIT License

## References

- [Adafruit MAX31865 Learning Guide](https://learn.adafruit.com/adafruit-max31865-rtd-pt100-amplifier)
- [MAX31865 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31865.pdf)
- [PT100 RTD Specifications](https://www.analog.com/en/resources/technical-articles/rtd-standards.html)
