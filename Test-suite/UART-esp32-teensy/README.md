# UART Communication: ESP32 DevKit to Teensy 4.1

This project demonstrates UART communication between an ESP32 DevKit and Teensy 4.1, with SD card logging on the Teensy.

## Wiring Diagram

```
    ESP32 DevKit                         Teensy 4.1
    ============                         ==========

    3.3V  [*]  [*]  VIN                    [*] VIN
    GND   [*]  [*]  GND -----------------> [*] GND
    GPIO15[*]  [*]  GPIO13                 [*] 3.3V
    GPIO2 [*]  [*]  GPIO12                 [*] 23
    GPIO4 [*]  [*]  GPIO14                 [*] 22
    GPIO16[*]<-[*]  GPIO27          +----> [*] 21
      RX2 |    [*]  GPIO26          |      [*] 20
          |    [*]  GPIO25          |      [*] 19
          |    [*]  GPIO33          |      [*] 18
          |    [*]  GPIO32          |      [*] 17
          |    [*]  GPIO35          |      [*] 16
          |    [*]  GPIO34          |      [*] 15
          |    [*]  VN              |      [*] 14
          |    [*]  VP              |      [*] 41
          |    [*]  EN              |      [*] 40
          |                         |      [*] 39
          |    [*]  [*]             |      [*] 38
          |    [*]  [*]  GPIO5      |      [*] 37
          |    [*]  [*]  GPIO18     |      [*] 36
          |    [*]  [*]  GPIO19     |      [*] 35
          |    [*]  [*]  GPIO21     |      [*] 34
          |    [*]  [*]  GPIO3      |      [*] 33
          |    [*]  [*]  GPIO1      |      [*] ...
    GPIO17[*]--+[*]  GPIO22         |
      TX2 |    [*]  GPIO23          |      [*] 2
          |                         |      [*] 1 (TX1) --+
          |                         +----> [*] 0 (RX1)   |
          +-------------------------------------------->-+
```

## Connection Table

| ESP32 DevKit | Direction | Teensy 4.1 | Function |
|--------------|-----------|------------|----------|
| GPIO17 (TX2) | --------> | Pin 0 (RX1)| UART Data (ESP32 to Teensy) |
| GPIO16 (RX2) | <-------- | Pin 1 (TX1)| UART Data (Teensy to ESP32) |
| GND          | <-------> | GND        | Common Ground |

**Important:** Both ESP32 and Teensy 4.1 operate at 3.3V logic levels - direct connection is safe without level shifters.

## Pinout Details

### ESP32 DevKit (Sender)
- **Serial** (USB): Debug output at 115200 baud
- **Serial2** (UART): Communication with Teensy
  - TX: GPIO17
  - RX: GPIO16
- **LED**: GPIO2 (built-in, blinks on transmit)

### Teensy 4.1 (Receiver)
- **Serial** (USB): Debug output at 115200 baud
- **Serial1** (UART): Communication with ESP32
  - RX: Pin 0
  - TX: Pin 1
- **LED**: Pin 13 (built-in, blinks on SD write)
- **SD Card**: Built-in slot (BUILTIN_SDCARD)

## Data Protocol

### Message Format
```
$<DEVICE>,<MSG_ID>,<TYPE>,<SENSOR>,<VALUE>,<UNIT>,<TIMESTAMP>*<CHECKSUM>\n
```

### Example Message
```
$esp32_sender,0001,DATA,TEMP,25.43,C,12345*A7
```

### Fields
| Field | Description | Example |
|-------|-------------|---------|
| DEVICE | Sender identifier | esp32_sender |
| MSG_ID | 4-digit message counter | 0001 |
| TYPE | Message type | DATA |
| SENSOR | Sensor name (TEMP/HUMID/BAT/PRES) | TEMP |
| VALUE | Sensor reading | 25.43 |
| UNIT | Unit of measurement | C |
| TIMESTAMP | Milliseconds since boot | 12345 |
| CHECKSUM | XOR checksum (2 hex digits) | A7 |

### ACK Response
```
ACK,<MSG_ID>,<STATUS>\n
```
Example: `ACK,0001,OK` or `ACK,0001,ERR`

## Building and Flashing

### ESP32
```bash
cd esp32dev
pio run              # Build only
pio run -t upload    # Build and flash
pio device monitor   # Open serial monitor
```

### Teensy 4.1
```bash
cd teensy4.1
pio run              # Build only
pio run -t upload    # Build and flash
pio device monitor   # Open serial monitor
```

## SD Card Log Format

Log file: `uart_log.txt`

```
=== UART Log Started ===
Boot time: 1234
Format: timestamp,validity,message
========================
5678,VALID,$esp32_sender,0001,DATA,TEMP,25.43,C,12345*A7
6789,VALID,$esp32_sender,0002,DATA,HUMID,62.15,%,13456*B8
```

## Verification Steps

1. **ESP32 Standalone Test**
   - Flash ESP32
   - Open serial monitor
   - Should see: `[TX] $esp32_sender,0001,DATA,TEMP,...`

2. **Teensy Standalone Test**
   - Insert SD card into Teensy
   - Flash Teensy
   - Open serial monitor
   - Should see: `Initializing SD card... OK`

3. **Combined Test**
   - Connect wires as shown above
   - Power both devices
   - ESP32 monitor shows: `[TX] ...` and `[RX] ACK,...`
   - Teensy monitor shows: `[RX] ...` and `[SD] Logged: VALID`

4. **SD Card Verification**
   - Power off Teensy
   - Remove SD card
   - Check `uart_log.txt` on computer

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No data received | TX/RX swapped | Check wiring matches diagram |
| Checksum errors | Noise/interference | Shorten wires, add ground |
| SD init failed | No card / bad card | Insert FAT32 formatted SD card |
| Garbage characters | Baud mismatch | Verify both use 115200 baud |

## Project Structure

```
UART-esp32-teensy/
├── README.md
├── esp32dev/
│   ├── platformio.ini
│   ├── include/
│   │   └── board_config.h
│   ├── lib/
│   │   └── DummySensors/
│   │       ├── DummySensors.h
│   │       └── DummySensors.cpp
│   └── src/
│       └── main.cpp
└── teensy4.1/
    ├── platformio.ini
    ├── include/
    │   └── board_config.h
    └── src/
        └── main.cpp
```
