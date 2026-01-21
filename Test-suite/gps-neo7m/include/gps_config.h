#ifndef GPS_CONFIG_H
#define GPS_CONFIG_H

// ============================================================================
// GPS NEO-7M Configuration
// ============================================================================
// Pin definitions are provided via build flags in platformio.ini
// This header validates that all required pins are defined at compile time

// ===== Pin Validation =====
#ifndef GPS_RX_PIN
    #error "GPS_RX_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef GPS_TX_PIN
    #error "GPS_TX_PIN not defined. Check platformio.ini build_flags"
#endif

#ifndef GPS_BAUD
    #define GPS_BAUD 9600  // Default NEO-7M baud rate
#endif

#ifndef BOARD_NAME
    #error "BOARD_NAME not defined. Check platformio.ini build_flags"
#endif

// ===== GPS Module Specifications =====
// NEO-7M GPS Module by u-blox
// - Input Voltage: 2.7V - 5.0V (module has onboard 3.3V regulator)
// - Logic Level: 3.3V (safe for direct connection to ESP32)
// - Default Baud Rate: 9600 bps
// - Communication: UART (8N1)
// - Update Rate: 1Hz (default), up to 10Hz configurable
// - Supported GNSS: GPS
// - Cold Start Time: ~27 seconds
// - Hot Start Time: ~1 second
// - Operating Current: 35-40mA

// ===== Wiring Guide =====
// NEO-7M Module     ESP32 Dev Board
// ==============    ===============
//     VCC  -------> 5V (VIN pin) - USE 5V, NOT 3.3V!
//     GND  -------> GND
//     TX   -------> GPIO16 (RX2) - GPS sends data to ESP32
//     RX   -------> GPIO17 (TX2) - ESP32 sends commands to GPS
//     PPS  -------> GPIO4 (optional) - 1Hz pulse for timing
//
// NOTE: The GPS LED should light up when properly powered!

// ===== Serial Configuration =====
#define SERIAL_BAUD 115200  // Serial monitor baud rate

// ===== GPS Data Update Intervals =====
#define GPS_DISPLAY_INTERVAL 2000   // Display GPS data every 2 seconds
#define GPS_DEBUG_INTERVAL   5000   // Debug statistics every 5 seconds

// ===== HDOP Quality Thresholds =====
// HDOP (Horizontal Dilution of Precision) indicates GPS accuracy
#define HDOP_IDEAL     1.0   // Ideal accuracy
#define HDOP_EXCELLENT 2.0   // Excellent accuracy
#define HDOP_GOOD      5.0   // Good accuracy
#define HDOP_MODERATE  10.0  // Moderate accuracy
#define HDOP_FAIR      20.0  // Fair accuracy
// > 20.0 = Poor accuracy

// ===== Minimum Satellites for Valid Fix =====
#define MIN_SATELLITES_2D 3  // Minimum for 2D fix (lat/lon only)
#define MIN_SATELLITES_3D 4  // Minimum for 3D fix (lat/lon/altitude)

#endif // GPS_CONFIG_H
