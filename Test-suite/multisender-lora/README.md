# LoRa Multi-Sender

Dual LoRa module transmitter - sends sensor data from two LoRa modules (trident1, trident2) on a single ESP32.

## Features

- Supports both **Ra-02 (SX1278)** and **SX1262** LoRa modules
- Two LoRa modules on one ESP32
- Alternating transmission between modules
- Device identification in each packet
- Rotates through all sensor types (Temperature, Humidity, Battery, Pressure)
- Build-flag based module selection

## Supported Configurations

| Board | Ra-02 (SX1278) | SX1262 |
|-------|----------------|--------|
| ESP32 DevKit V1 | `esp32dev_dual_ra02` | `esp32dev_dual_sx1262` |

**Note:** Arduino UNO is not supported due to limited RAM (2KB) and single SPI bus.

## Wiring Diagram

### ESP32 DevKit + 2x Ra-02 (SX1278) Modules

```
ESP32 DevKit          LoRa Module 1 (trident1)     LoRa Module 2 (trident2)
-----------           ----------------------        ----------------------
GPIO 5  (NSS1)  ----> NSS
GPIO 15 (NSS2)  ----------------------------->     NSS
GPIO 14 (DIO0_1) ---> DIO0
GPIO 27 (DIO0_2) ---------------------------->     DIO0
GPIO 21 (RST1)  ---> RESET
GPIO 26 (RST2)  ----------------------------->     RESET
GPIO 18 (SCK)   ----> SCK  ------------------>     SCK    (shared)
GPIO 19 (MISO)  <---- MISO <------------------     MISO   (shared)
GPIO 23 (MOSI)  ----> MOSI ------------------>     MOSI   (shared)
3.3V            ----> VCC  ------------------>     VCC
GND             ----> GND  ------------------>     GND
```

### ESP32 DevKit + 2x SX1262 Modules

```
ESP32 DevKit          LoRa Module 1 (trident1)     LoRa Module 2 (trident2)
-----------           ----------------------        ----------------------
GPIO 5  (NSS1)  ----> NSS
GPIO 15 (NSS2)  ----------------------------->     NSS
GPIO 14 (DIO1_1) ---> DIO1
GPIO 27 (DIO1_2) ---------------------------->     DIO1
GPIO 21 (RST1)  ---> RESET
GPIO 26 (RST2)  ----------------------------->     RESET
GPIO 4  (BUSY1) <--- BUSY
GPIO 25 (BUSY2) <---------------------------      BUSY
GPIO 18 (SCK)   ----> SCK  ------------------>     SCK    (shared)
GPIO 19 (MISO)  <---- MISO <------------------     MISO   (shared)
GPIO 23 (MOSI)  ----> MOSI ------------------>     MOSI   (shared)
3.3V            ----> VCC  ------------------>     VCC
GND             ----> GND  ------------------>     GND
```

## Pin Summary Table

### Ra-02 (SX1278)

| Signal | ESP32 DevKit | Module 1 | Module 2 |
|--------|--------------|----------|----------|
| NSS    | GPIO 5       | Yes      | -        |
| NSS    | GPIO 15      | -        | Yes      |
| DIO0   | GPIO 14      | Yes      | -        |
| DIO0   | GPIO 27      | -        | Yes      |
| RESET  | GPIO 21      | Yes      | -        |
| RESET  | GPIO 26      | -        | Yes      |
| SCK    | GPIO 18      | Shared   | Shared   |
| MISO   | GPIO 19      | Shared   | Shared   |
| MOSI   | GPIO 23      | Shared   | Shared   |

### SX1262

| Signal | ESP32 DevKit | Module 1 | Module 2 |
|--------|--------------|----------|----------|
| NSS    | GPIO 5       | Yes      | -        |
| NSS    | GPIO 15      | -        | Yes      |
| DIO1   | GPIO 14      | Yes      | -        |
| DIO1   | GPIO 27      | -        | Yes      |
| RESET  | GPIO 21      | Yes      | -        |
| RESET  | GPIO 26      | -        | Yes      |
| BUSY   | GPIO 4       | Yes      | -        |
| BUSY   | GPIO 25      | -        | Yes      |
| SCK    | GPIO 18      | Shared   | Shared   |
| MISO   | GPIO 19      | Shared   | Shared   |
| MOSI   | GPIO 23      | Shared   | Shared   |

## Build & Upload

```bash
# Build for Ra-02 modules (default)
pio run -e esp32dev_dual_ra02

# Build for SX1262 modules
pio run -e esp32dev_dual_sx1262

# Upload Ra-02 firmware
pio run -e esp32dev_dual_ra02 -t upload

# Upload SX1262 firmware
pio run -e esp32dev_dual_sx1262 -t upload

# Monitor serial output
pio device monitor
```

## Expected Output

```
==========================================
  LoRa MULTI-SENDER
  Dual Module Transmitter
==========================================
Board: ESP32Dev

=== Dual LoRa Module Initialization ===
Module Type: Ra-02 (SX1278)
...
=== Both modules initialized successfully ===

[TX] [trident1] Temperature: 25.34 °C (20 bytes)
[TX] [trident2] Humidity: 65.20 % (18 bytes)
[TX] [trident1] Battery: 3.85 V (17 bytes)
[TX] [trident2] Pressure: 1013.25 hPa (21 bytes)
```

## Receiver Output

When using the updated receiver, you'll see:

```
[5s] [trident1] Temperature: 25.34 °C | RSSI: -35 dBm | SNR: 9.5 dB | ID: 1
[10s] [trident2] Humidity: 65.20 % | RSSI: -33 dBm | SNR: 9.8 dB | ID: 2
[15s] [trident1] Battery: 3.85 V | RSSI: -36 dBm | SNR: 9.2 dB | ID: 3
```

## Customizing Device Names

Edit `platformio.ini` to change device names:

```ini
build_flags =
    ...
    -D LORA1_NAME=\"mydevice1\"
    -D LORA2_NAME=\"mydevice2\"
```

## Key Differences: Ra-02 vs SX1262

| Feature | Ra-02 (SX1278) | SX1262 |
|---------|----------------|--------|
| Interrupt Pin | DIO0 | DIO1 |
| BUSY Pin | Not used | Required |
| Power Efficiency | Good | Better |
| Range | Good | Better |

## Cross-Chip Communication

Ra-02 and SX1262 modules can communicate with each other as long as they use the same:
- Frequency (433 MHz)
- Spreading Factor (SF7)
- Bandwidth (125 kHz)
- Coding Rate (4/5)
- Sync Word (0x12)
