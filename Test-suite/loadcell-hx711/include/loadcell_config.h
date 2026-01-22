#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

// ============================================================================
// HX711 Load Cell Configuration
// ============================================================================
// Pin definitions are provided via build flags in platformio.ini
// This header validates that all required pins are defined at compile time

// ===== Pin Validation =====
#ifndef LOADCELL_DOUT_PIN
    #error "LOADCELL_DOUT_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef LOADCELL_SCK_PIN
    #error "LOADCELL_SCK_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef BOARD_NAME
    #error "BOARD_NAME not defined. Check platformio.ini build_flags"
#endif

// ===== HX711 Specifications =====
// HX711 24-Bit ADC for Load Cells
// - Input Voltage: 2.6V - 5.5V
// - Operating Current: < 1.5mA (normal), < 1uA (power down)
// - Data Rate: 10Hz (RATE pin low) or 80Hz (RATE pin high)
// - Resolution: 24-bit (effective 20-bit at 10Hz)
// - Gain Options: 128 (Channel A), 64 (Channel A), 32 (Channel B)
// - Default Gain: 128 (Channel A)

// ===== Wiring Guide =====
// HX711 Module       ESP32 Dev Board
// ==============     ===============
//     VCC  -------> 3.3V
//     GND  -------> GND
//     DT   -------> GPIO16 (Data Out)
//     SCK  -------> GPIO4  (Serial Clock)
//
// Load Cell to HX711:
//     Red   (E+) -> HX711 E+  (Excitation+)
//     Black (E-) -> HX711 E-  (Excitation-)
//     White (A-) -> HX711 A-  (Signal-)
//     Green (A+) -> HX711 A+  (Signal+)
//
// NOTE: Wire colors may vary between manufacturers!
//       Common alternatives:
//       - Red/Black for excitation (power)
//       - White/Green or Blue/Yellow for signal

// ===== Serial Configuration =====
#define SERIAL_BAUD 115200  // Serial monitor baud rate

// ===== Load Cell Configuration =====
// Calibration factor - MUST be determined through calibration!
// Default value is approximate, run calibration mode to find your value
// Formula: calibration_factor = raw_reading / known_weight
#ifndef CALIBRATION_FACTOR
    #define CALIBRATION_FACTOR -471.497  // Example value, calibrate for your setup!
#endif

// Number of readings to average for more stable results
#define READINGS_TO_AVERAGE 10

// ===== Measurement Intervals =====
#define DISPLAY_INTERVAL    1000   // Display weight every 1 second
#define STABILITY_INTERVAL  5000   // Check stability every 5 seconds

// ===== Stability Detection =====
// Weight is considered stable if change is less than threshold
#define STABILITY_THRESHOLD 0.5    // Weight change threshold in grams
#define STABILITY_SAMPLES   5      // Number of samples to check for stability

// ===== Tare Configuration =====
#define TARE_READINGS 20  // Number of readings for tare operation

// ===== Weight Thresholds =====
#define MIN_WEIGHT_THRESHOLD 0.5   // Ignore weights below this (noise filter)
#define MAX_WEIGHT_CAPACITY  5000  // Maximum expected weight in grams

// ===== Operating Modes =====
#define MODE_NORMAL      0   // Normal weight measurement
#define MODE_CALIBRATION 1   // Calibration mode
#define MODE_RAW         2   // Raw ADC readings mode

#endif // LOADCELL_CONFIG_H
