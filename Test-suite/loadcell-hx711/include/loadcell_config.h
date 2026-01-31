#ifndef LOADCELL_CONFIG_H
#define LOADCELL_CONFIG_H

// ============================================================================
// HX711 Load Cell Configuration - THRUST TEST MODE
// ============================================================================
// High-speed (80Hz) thrust measurement for rocket motor testing
// Output in Newtons, bidirectional (tension/compression)

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
// - Data Rate: 80Hz (RATE pin HIGH) - REQUIRED for thrust testing
// - Resolution: 24-bit ADC
// - Gain: 128 (Channel A, default)
//
// HARDWARE REQUIREMENT:
// For 80Hz mode, modify HX711 board:
//   - Cut RATE pin trace to GND, OR
//   - Bridge RATE pin to VCC
// Default 10Hz is too slow for thrust curves!

// ===== Serial Configuration =====
// High-speed serial for 80Hz output without bottleneck
#define SERIAL_BAUD 921600

// ===== Physical Constants =====
// Conversion from grams to Newtons
// 1 gram-force = 0.00980665 Newtons (standard gravity)
#define GRAMS_TO_NEWTONS 0.00980665f

// ===== Load Cell Configuration =====
// Calibration factor - determines force reading accuracy
// This factor converts raw ADC to Newtons directly
// Run calibration with known weight to find your value
// Formula: cal_factor = raw_difference / known_force_N
#ifndef CALIBRATION_FACTOR
#define CALIBRATION_FACTOR 1496.0f  // Calibrate for your setup!
#endif

// ===== Tare Configuration =====
#define TARE_READINGS 20  // Number of readings for tare (zero) operation

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

#endif // LOADCELL_CONFIG_H
