# LoRa Receiver

Single LoRa module receiver - receives and displays sensor data packets over LoRa.

## Features

- Supports both **Ra-02 (SX1278)** and **SX1262** LoRa modules
- Build-flag based module selection
- Displays RSSI and SNR for each packet
- LED indicator on packet reception
- Compatible with sender-lora and multisender-lora

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
GPIO 2          ----> LED (onboard)
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
GPIO 2          ----> LED (onboard)
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
Pin 13          ----> LED (onboard)
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
| LED    | GPIO 2    | GPIO 2     |

### Arduino Uno (Ra-02 only)

| Signal | Pin |
|--------|-----|
| NSS    | 10  |
| DIO0   | 2   |
| RESET  | 9   |
| SCK    | 13  |
| MISO   | 12  |
| MOSI   | 11  |
| LED    | 13  |

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
...

Waiting for packets...

[5s] [sender1] Temperature: 25.34 °C | RSSI: -35 dBm | SNR: 9.5 dB | ID: 1
[10s] [sender1] Humidity: 65.20 % | RSSI: -33 dBm | SNR: 9.8 dB | ID: 2
[15s] [trident1] Battery: 3.85 V | RSSI: -36 dBm | SNR: 9.2 dB | ID: 3
```

## LED Behavior

- **Blink on receive:** LED flashes briefly when a valid packet is received
- **Solid on:** Indicates active reception mode

## Key Differences: Ra-02 vs SX1262

| Feature | Ra-02 (SX1278) | SX1262 |
|---------|----------------|--------|
| Interrupt Pin | DIO0 | DIO1 |
| BUSY Pin | Not used | Required |
| Power Efficiency | Good | Better |
| Receive Sensitivity | Good | Better |

## Cross-Chip Communication

Ra-02 and SX1262 modules can communicate with each other as long as they use the same LoRa parameters:
- Frequency: 433 MHz
- Spreading Factor: SF7
- Bandwidth: 125 kHz
- Coding Rate: 4/5
- Sync Word: 0x12
