#ifndef RTD_CONFIG_H
#define RTD_CONFIG_H

// ============================================================================
// MAX31865 RTD Temperature Sensor Configuration
// ============================================================================
// Pin definitions are provided via build flags in platformio.ini
// This header validates that all required pins are defined at compile time
//
// NOTE: The MAX31865 is an RTD (Resistance Temperature Detector) amplifier,
//       designed for PT100/PT1000 sensors. It is NOT a thermocouple amplifier.
//       For thermocouples, use MAX31855 or MAX31856.

// ===== Pin Validation =====
#ifndef MAX31865_CS_PIN
    #error "MAX31865_CS_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef MAX31865_MOSI_PIN
    #error "MAX31865_MOSI_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef MAX31865_MISO_PIN
    #error "MAX31865_MISO_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef MAX31865_CLK_PIN
    #error "MAX31865_CLK_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef BOARD_NAME
    #error "BOARD_NAME not defined. Check platformio.ini build_flags"
#endif

// ===== MAX31865 Specifications =====
// MAX31865 RTD-to-Digital Converter
// - Input Voltage: 3.0V - 3.6V
// - ADC Resolution: 15-bit
// - Temperature Resolution: ~0.03125°C
// - Conversion Time: 21ms typical
// - Interface: SPI (up to 5MHz)
// - Supports: 2-wire, 3-wire, 4-wire RTD configurations

// ===== RTD Sensor Types =====
// PT100:  100 ohms at 0°C, Reference Resistor: 430 ohms
// PT1000: 1000 ohms at 0°C, Reference Resistor: 4300 ohms

// ===== Default RTD Configuration =====
// Set RTD_TYPE to 100 for PT100 or 1000 for PT1000
#ifndef RTD_TYPE
    #define RTD_TYPE 100  // PT100 sensor
#endif

// Reference resistor value (must match your board)
// PT100 boards typically have 430 ohm, PT1000 boards have 4300 ohm
#ifndef RTD_RREF
    #if RTD_TYPE == 100
        #define RTD_RREF 430.0
    #else
        #define RTD_RREF 4300.0
    #endif
#endif

// RTD nominal resistance at 0°C
#ifndef RTD_RNOMINAL
    #define RTD_RNOMINAL RTD_TYPE
#endif

// ===== RTD Wiring Configuration =====
// 2 = 2-wire RTD
// 3 = 3-wire RTD (most common)
// 4 = 4-wire RTD (highest accuracy)
#ifndef RTD_WIRES
    #define RTD_WIRES 3
#endif

// ===== Wiring Guide =====
// MAX31865 Module      ESP32 Dev Board (VSPI)
// ==============       ======================
//     VIN   -------> 3.3V
//     GND   -------> GND
//     CLK   -------> GPIO18 (VSPI CLK)
//     SDO   -------> GPIO19 (VSPI MISO)
//     SDI   -------> GPIO23 (VSPI MOSI)
//     CS    -------> GPIO5  (VSPI SS)
//
// RTD Sensor to MAX31865 (3-Wire Configuration):
//     RTD Wire 1 -> RTD+
//     RTD Wire 2 -> RTD-
//     RTD Wire 3 -> F-
//     (Jumper F+ to RTD+ on some boards)
//
// NOTE: Check your specific breakout board documentation!
//       Adafruit boards have solder jumpers for 2/3/4-wire selection.

// ===== Serial Configuration =====
#define SERIAL_BAUD 115200  // Serial monitor baud rate

// ===== Temperature Thresholds =====
// Operating range for PT100/PT1000 sensors
#define TEMP_MIN -200.0    // Minimum measurable temperature (°C)
#define TEMP_MAX 850.0     // Maximum measurable temperature (°C)

// Alert thresholds (adjust for your application)
#ifndef TEMP_ALERT_LOW
    #define TEMP_ALERT_LOW -40.0   // Low temperature alert threshold
#endif

#ifndef TEMP_ALERT_HIGH
    #define TEMP_ALERT_HIGH 100.0  // High temperature alert threshold
#endif

// ===== Measurement Configuration =====
#define DISPLAY_INTERVAL     1000   // Display temperature every 1 second
#define STABILITY_INTERVAL   5000   // Check stability every 5 seconds

// ===== Stability Detection =====
// Temperature is considered stable if change is less than threshold
#define STABILITY_THRESHOLD  0.5    // Temperature change threshold in °C
#define STABILITY_SAMPLES    5      // Number of samples to check for stability

// ===== Averaging Configuration =====
#define READINGS_TO_AVERAGE  10     // Number of readings to average

// ===== Fault Detection =====
// MAX31865 can detect various fault conditions:
// - RTD High Threshold
// - RTD Low Threshold
// - REFIN- > 0.85 x VBIAS
// - REFIN- < 0.85 x VBIAS (FORCE- open)
// - RTDIN- < 0.85 x VBIAS (FORCE- open)
// - Overvoltage/Undervoltage

#endif // RTD_CONFIG_H
