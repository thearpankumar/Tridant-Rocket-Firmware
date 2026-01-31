# LoRa Sender

Single LoRa module transmitter - sends sensor data packets over LoRa.

## Features

- Supports both **Ra-02 (SX1278)** and **SX1262** LoRa modules
- Build-flag based module selection
- Dummy sensor data generation for testing
- Device identification in packets
- Compatible with receiver-lora

## Supported Configurations

| Board | Ra-02 (SX1278) | SX1262 |
|-------|----------------|--------|
| ESP32 DevKit V1 | `esp32dev_ra02` | `esp32dev_sx1262` |
| Arduino Uno | `uno_ra02` | Not supported |

## Wiring Diagram

### ESP32 DevKit + Ra-02 (SX1278)

```
ESP32 DevKit          Ra-02 Module
-----------           ------------
GPIO 5  (NSS)   ----> NSS
GPIO 14 (DIO0)  ----> DIO0
GPIO 21 (RST)   ----> RESET
GPIO 18 (SCK)   ----> SCK
GPIO 19 (MISO)  <---- MISO
GPIO 23 (MOSI)  ----> MOSI
3.3V            ----> VCC
GND             ----> GND
```

### ESP32 DevKit + SX1262

```
ESP32 DevKit          SX1262 Module
-----------           -------------
GPIO 5  (NSS)   ----> NSS
GPIO 14 (DIO1)  ----> DIO1
GPIO 21 (RST)   ----> RESET
GPIO 4  (BUSY)  <---- BUSY
GPIO 18 (SCK)   ----> SCK
GPIO 19 (MISO)  <---- MISO
GPIO 23 (MOSI)  ----> MOSI
3.3V            ----> VCC
GND             ----> GND
```

### Arduino Uno + Ra-02 (requires level shifters!)

```
Arduino Uno           Ra-02 Module
-----------           ------------
Pin 10 (NSS)    ----> NSS    (via level shifter)
Pin 2  (DIO0)   ----> DIO0   (via level shifter)
Pin 9  (RST)    ----> RESET  (via level shifter)
Pin 13 (SCK)    ----> SCK    (via level shifter)
Pin 12 (MISO)   <---- MISO
Pin 11 (MOSI)   ----> MOSI   (via level shifter)
3.3V            ----> VCC
GND             ----> GND
```

**Warning:** Arduino Uno is 5V logic. You MUST use level shifters for all signals going to the Ra-02 (which is 3.3V only).

## Pin Summary Table

### ESP32 DevKit

| Signal | Ra-02 Pin | SX1262 Pin |
|--------|-----------|------------|
| NSS    | GPIO 5    | GPIO 5     |
| DIO0   | GPIO 14   | -          |
| DIO1   | -         | GPIO 14    |
| BUSY   | -         | GPIO 4     |
| RESET  | GPIO 21   | GPIO 21    |
| SCK    | GPIO 18   | GPIO 18    |
| MISO   | GPIO 19   | GPIO 19    |
| MOSI   | GPIO 23   | GPIO 23    |

### Arduino Uno (Ra-02 only)

| Signal | Pin |
|--------|-----|
| NSS    | 10  |
| DIO0   | 2   |
| RESET  | 9   |
| SCK    | 13  |
| MISO   | 12  |
| MOSI   | 11  |

## Build & Upload

```bash
# Build for Ra-02 (default)
pio run -e esp32dev_ra02

# Build for SX1262
pio run -e esp32dev_sx1262

# Build for Arduino Uno (Ra-02 only)
pio run -e uno_ra02

# Upload Ra-02 firmware to ESP32
pio run -e esp32dev_ra02 -t upload

# Upload SX1262 firmware to ESP32
pio run -e esp32dev_sx1262 -t upload

# Monitor serial output
pio device monitor
```

## Expected Output

```
=== LoRa Initialization Debug ===
Module Type: Ra-02 (SX1278)
NSS (CS): GPIO 5
RESET: GPIO 21
DIO0: GPIO 14
Frequency: 433.00 MHz
Initializing radio module...
Radio module initialized successfully!
SUCCESS: LoRa module initialized
--- LoRa Configuration ---
Module: Ra-02 (SX1278)
Frequency: 433.00 MHz
Spreading Factor: SF7
Bandwidth: 125.00 kHz
...
[TX] Temperature: 25.34 °C (20 bytes)
[TX] Humidity: 65.20 % (18 bytes)
```

## Customizing Device Name

Edit `platformio.ini`:

```ini
build_flags =
    ...
    -D DEVICE_NAME=\"mydevice\"
```

## Key Differences: Ra-02 vs SX1262

| Feature | Ra-02 (SX1278) | SX1262 |
|---------|----------------|--------|
| Interrupt Pin | DIO0 | DIO1 |
| BUSY Pin | Not used | Required |
| Power Efficiency | Good | Better |
| Range | Good | Better |

## Cross-Chip Communication

Ra-02 and SX1262 modules can communicate with each other as long as they use the same LoRa parameters (frequency, SF, BW, CR, sync word).
