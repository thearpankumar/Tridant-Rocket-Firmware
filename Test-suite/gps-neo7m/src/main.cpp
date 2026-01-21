#include <Arduino.h>
#include "GPSModule.h"
#include "gps_config.h"

// ===== Debug Mode =====
// Set to true to see raw NMEA sentences from GPS
#define DEBUG_RAW_NMEA true

// ===== Global Objects =====
GPSModule gps;

// ===== Timing Variables =====
unsigned long lastDisplayTime = 0;
unsigned long lastDebugTime = 0;
unsigned long lastRawTime = 0;

// ===== Function Prototypes =====
void displayGPSData();
void displayDebugInfo();
void displayRawNMEA();
void printSeparator();

void setup() {
    // Initialize Serial Monitor
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    printSeparator();
    Serial.println(F("  GPS NEO-7M Module Test"));
    Serial.println(F("  ESP32 Dev Board"));
    printSeparator();
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.println();

    // Print wiring guide - USE 5V FOR POWER!
    Serial.println(F("=== Wiring Guide ==="));
    Serial.println(F("NEO-7M      ESP32"));
    Serial.println(F("------      -----"));
    Serial.println(F("VCC    -->  5V (VIN pin)"));
    Serial.println(F("GND    -->  GND"));
    Serial.print(F("TX     -->  GPIO"));
    Serial.print(GPS_RX_PIN);
    Serial.println(F(" (RX2)"));
    Serial.print(F("RX     -->  GPIO"));
    Serial.print(GPS_TX_PIN);
    Serial.println(F(" (TX2)"));
    #ifdef GPS_PPS_PIN
    Serial.print(F("PPS    -->  GPIO"));
    Serial.print(GPS_PPS_PIN);
    Serial.println(F(" (optional)"));
    #endif
    Serial.println();

    // Initialize GPS module
    Serial.println(F("Initializing GPS module..."));
    Serial.print(F("RX Pin: GPIO"));
    Serial.println(GPS_RX_PIN);
    Serial.print(F("TX Pin: GPIO"));
    Serial.println(GPS_TX_PIN);
    Serial.print(F("Baud Rate: "));
    Serial.println(GPS_BAUD);

    if (!gps.begin(GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD)) {
        Serial.println(F("\nFATAL: GPS initialization failed!"));
        Serial.println(F("System halted. Check wiring and reset board."));
        while (1) {
            delay(1000);
        }
    }

    Serial.println(F("GPS module initialized successfully!"));
    Serial.println();

    #if DEBUG_RAW_NMEA
    Serial.println(F("DEBUG MODE: Raw NMEA output enabled"));
    Serial.println(F("You should see $GPGGA, $GPRMC sentences below"));
    Serial.println(F("If you see garbage or nothing, check wiring/power"));
    #endif

    printSeparator();
    Serial.println(F("  Waiting for GPS Fix..."));
    Serial.println(F("  (Place module outdoors or near window)"));
    Serial.println(F("  Cold start may take 1-2 minutes"));
    printSeparator();
    Serial.println();
}

void loop() {
    // Continuously update GPS data
    gps.update();

    unsigned long currentTime = millis();

    // Print raw NMEA data for debugging
    #if DEBUG_RAW_NMEA
    if (currentTime - lastRawTime >= 3000) {  // Every 3 seconds
        lastRawTime = currentTime;
        displayRawNMEA();
    }
    #endif

    // Display GPS data at regular intervals
    if (currentTime - lastDisplayTime >= GPS_DISPLAY_INTERVAL) {
        lastDisplayTime = currentTime;
        displayGPSData();
    }

    // Display debug info less frequently
    if (currentTime - lastDebugTime >= GPS_DEBUG_INTERVAL) {
        lastDebugTime = currentTime;
        displayDebugInfo();
    }
}

void displayGPSData() {
    GPSData data = gps.getData();

    Serial.println(F("========== GPS Data =========="));

    // Fix Status
    Serial.print(F("Fix Status: "));
    Serial.print(gps.getFixQuality());
    if (data.satellitesValid) {
        Serial.print(F(" ("));
        Serial.print(data.satellites);
        Serial.println(F(" satellites)"));
    } else {
        Serial.println();
    }

    // Location
    Serial.print(F("Location:   "));
    if (data.locationValid) {
        Serial.print(data.latitude, 6);
        Serial.print(F(", "));
        Serial.println(data.longitude, 6);
    } else {
        Serial.println(F("Waiting for fix..."));
    }

    // Altitude
    Serial.print(F("Altitude:   "));
    if (data.altitudeValid) {
        Serial.print(data.altitude, 1);
        Serial.println(F(" m"));
    } else {
        Serial.println(F("N/A"));
    }

    // Speed
    Serial.print(F("Speed:      "));
    if (data.speedValid) {
        Serial.print(data.speedKmph, 1);
        Serial.print(F(" km/h ("));
        Serial.print(data.speedMps, 1);
        Serial.println(F(" m/s)"));
    } else {
        Serial.println(F("N/A"));
    }

    // Course/Heading
    Serial.print(F("Course:     "));
    if (data.courseValid) {
        Serial.print(data.course, 1);
        Serial.println(F(" deg"));
    } else {
        Serial.println(F("N/A"));
    }

    // HDOP (accuracy)
    Serial.print(F("HDOP:       "));
    if (data.hdopValid) {
        Serial.print(data.hdop, 2);
        Serial.print(F(" ("));
        Serial.print(gps.getHDOPQuality());
        Serial.println(F(")"));
    } else {
        Serial.println(F("N/A"));
    }

    // Date/Time
    Serial.print(F("Date/Time:  "));
    if (data.dateValid && data.timeValid) {
        char dateTimeStr[25];
        gps.getDateTimeString(dateTimeStr);
        Serial.println(dateTimeStr);
    } else {
        Serial.println(F("Waiting for time sync..."));
    }

    // Fix Age
    if (data.locationValid) {
        Serial.print(F("Fix Age:    "));
        Serial.print(gps.getFixAge());
        Serial.println(F(" ms"));
    }

    Serial.println(F("=============================="));
    Serial.println();
}

void displayDebugInfo() {
    Serial.println(F("----- Debug Statistics -----"));
    Serial.print(F("Characters processed: "));
    Serial.println(gps.getCharsProcessed());
    Serial.print(F("Sentences with fix:   "));
    Serial.println(gps.getSentencesWithFix());
    Serial.print(F("Failed checksums:     "));
    Serial.println(gps.getFailedChecksums());

    // Warning if no data received
    if (gps.getCharsProcessed() == 0) {
        Serial.println();
        Serial.println(F("WARNING: No data received from GPS!"));
        Serial.println(F("Check wiring:"));
        Serial.print(F("  - GPS TX -> ESP32 GPIO"));
        Serial.println(GPS_RX_PIN);
        Serial.print(F("  - GPS RX -> ESP32 GPIO"));
        Serial.println(GPS_TX_PIN);
        Serial.println(F("  - GPS VCC -> 3.3V"));
        Serial.println(F("  - GPS GND -> GND"));
    }

    // Warning if many failed checksums
    if (gps.getFailedChecksums() > 10) {
        Serial.println();
        Serial.println(F("WARNING: High checksum failures!"));
        Serial.println(F("Possible causes:"));
        Serial.println(F("  - Electrical noise"));
        Serial.println(F("  - Incorrect baud rate"));
        Serial.println(F("  - Loose connections"));
    }

    Serial.println(F("----------------------------"));
    Serial.println();
}

void displayRawNMEA() {
    Serial.println(F("--- Raw NMEA Data (next 500ms) ---"));
    unsigned long start = millis();
    bool gotData = false;

    // Read raw data for 500ms
    while (millis() - start < 500) {
        while (gps.available()) {
            char c = gps.readRaw();
            Serial.write(c);
            gotData = true;
        }
    }

    if (!gotData) {
        Serial.println(F("[NO DATA RECEIVED]"));
        Serial.println(F("Check: GPS VCC -> ESP32 5V (not 3.3V!)"));
        Serial.println(F("Check: GPS TX -> ESP32 GPIO16"));
        Serial.println(F("Check: GPS GND -> ESP32 GND"));
    }
    Serial.println();
    Serial.println(F("----------------------------------"));
}

void printSeparator() {
    Serial.println(F("===================================="));
}
