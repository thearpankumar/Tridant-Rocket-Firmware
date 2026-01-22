#include <Arduino.h>
#include <math.h>
#include "LoadCellModule.h"
#include "loadcell_config.h"

// ===== Operating Mode =====
// Set to true to run calibration mode, false for normal operation
#define CALIBRATION_MODE false

// Known weight for calibration (in grams)
#define KNOWN_WEIGHT 100.0

// ===== Global Objects =====
LoadCellModule loadCell;

// ===== Timing Variables =====
unsigned long lastDisplayTime = 0;
unsigned long lastStabilityTime = 0;

// ===== Function Prototypes =====
void runNormalMode();
void runCalibrationMode();
void displayWeight();
void displayStabilityStatus();
void printSeparator();
void printWiringGuide();

void setup() {
    // Initialize Serial Monitor
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    printSeparator();
    Serial.println(F("  HX711 Load Cell Test"));
    Serial.println(F("  ESP32 Dev Board"));
    printSeparator();
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.println();

    // Print wiring guide
    printWiringGuide();

    // Initialize load cell
    Serial.println(F("Initializing HX711..."));
    Serial.print(F("DOUT Pin: GPIO"));
    Serial.println(LOADCELL_DOUT_PIN);
    Serial.print(F("SCK Pin:  GPIO"));
    Serial.println(LOADCELL_SCK_PIN);
    Serial.println();

    if (!loadCell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, CALIBRATION_FACTOR)) {
        Serial.println(F("\nFATAL: HX711 initialization failed!"));
        Serial.println(F("Check wiring:"));
        Serial.println(F("  - Is HX711 powered? (VCC -> 3.3V)"));
        Serial.println(F("  - Are data pins connected correctly?"));
        Serial.println(F("  - Is load cell wired to HX711?"));
        Serial.println(F("System halted."));
        while (1) {
            delay(1000);
        }
    }

    Serial.println(F("HX711 initialized successfully!"));
    Serial.println();

    // Configure stability detection
    loadCell.setStabilityThreshold(STABILITY_THRESHOLD);
    loadCell.setStabilitySamples(STABILITY_SAMPLES);

    #if CALIBRATION_MODE
    Serial.println(F("=== CALIBRATION MODE ==="));
    Serial.println(F("Follow the instructions to calibrate your load cell."));
    #else
    Serial.print(F("Calibration Factor: "));
    Serial.println(CALIBRATION_FACTOR);
    Serial.println();

    // Tare the scale
    Serial.println(F("Taring scale... Remove all weight!"));
    delay(2000);
    loadCell.tare(TARE_READINGS);
    Serial.println(F("Tare complete!"));
    #endif

    printSeparator();
    Serial.println();
}

void loop() {
    #if CALIBRATION_MODE
    runCalibrationMode();
    #else
    runNormalMode();
    #endif
}

void runNormalMode() {
    // Update load cell data
    loadCell.update();

    unsigned long currentTime = millis();

    // Display weight at regular intervals
    if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = currentTime;
        displayWeight();
    }

    // Display stability status periodically
    if (currentTime - lastStabilityTime >= STABILITY_INTERVAL) {
        lastStabilityTime = currentTime;
        displayStabilityStatus();
    }

    // Check for serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 't':
            case 'T':
                Serial.println(F("\nTaring... Remove all weight!"));
                delay(1000);
                loadCell.tare(TARE_READINGS);
                Serial.println(F("Tare complete!\n"));
                break;

            case 'r':
            case 'R':
                Serial.println(F("\n--- Raw ADC Reading ---"));
                Serial.print(F("Raw Value: "));
                Serial.println(loadCell.getRawValue());
                Serial.print(F("Avg Raw (10): "));
                Serial.println(loadCell.getAverageRawValue(10));
                Serial.println();
                break;

            case 'c':
            case 'C':
                Serial.print(F("\nCalibration Factor: "));
                Serial.println(loadCell.getCalibrationFactor());
                Serial.println();
                break;

            case 'h':
            case 'H':
            case '?':
                Serial.println(F("\n=== Commands ==="));
                Serial.println(F("t - Tare (zero) the scale"));
                Serial.println(F("r - Show raw ADC reading"));
                Serial.println(F("c - Show calibration factor"));
                Serial.println(F("h - Show this help"));
                Serial.println();
                break;
        }
    }
}

void runCalibrationMode() {
    static uint8_t step = 0;
    static float rawNoWeight = 0;
    static float rawWithWeight = 0;

    switch (step) {
        case 0:
            Serial.println(F("\n=== Step 1: Tare ==="));
            Serial.println(F("Remove all weight from the load cell."));
            Serial.println(F("Press ENTER when ready..."));
            step = 1;
            break;

        case 1:
            if (Serial.available()) {
                Serial.read();  // Consume input
                Serial.println(F("Measuring zero point..."));
                loadCell.tare(TARE_READINGS);
                rawNoWeight = loadCell.getAverageRawValue(20);
                Serial.print(F("Zero reading: "));
                Serial.println(rawNoWeight);
                Serial.println(F("Done!\n"));
                step = 2;
            }
            break;

        case 2:
            Serial.println(F("=== Step 2: Known Weight ==="));
            Serial.print(F("Place a known weight of "));
            Serial.print(KNOWN_WEIGHT);
            Serial.println(F(" grams on the scale."));
            Serial.println(F("Press ENTER when ready..."));
            step = 3;
            break;

        case 3:
            if (Serial.available()) {
                Serial.read();  // Consume input
                Serial.println(F("Measuring known weight..."));
                rawWithWeight = loadCell.getAverageRawValue(20);
                Serial.print(F("Raw reading: "));
                Serial.println(rawWithWeight);
                step = 4;
            }
            break;

        case 4: {
            Serial.println(F("\n=== Calibration Complete ==="));

            // Calculate calibration factor
            float calFactor = rawWithWeight / KNOWN_WEIGHT;

            Serial.print(F("Raw reading (no weight): "));
            Serial.println(rawNoWeight);
            Serial.print(F("Raw reading ("));
            Serial.print(KNOWN_WEIGHT);
            Serial.print(F("g): "));
            Serial.println(rawWithWeight);
            Serial.println();

            Serial.println(F("*** YOUR CALIBRATION FACTOR ***"));
            printSeparator();
            Serial.print(F("CALIBRATION_FACTOR = "));
            Serial.println(calFactor, 3);
            printSeparator();
            Serial.println();

            Serial.println(F("Update this value in loadcell_config.h:"));
            Serial.print(F("  #define CALIBRATION_FACTOR "));
            Serial.println(calFactor, 3);
            Serial.println();

            Serial.println(F("Or in platformio.ini build_flags:"));
            Serial.print(F("  -D CALIBRATION_FACTOR="));
            Serial.println(calFactor, 3);
            Serial.println();

            // Apply and test
            Serial.println(F("Testing with new calibration factor..."));
            loadCell.setCalibrationFactor(calFactor);
            loadCell.tare(TARE_READINGS);

            Serial.println(F("\nPlace different weights to verify calibration."));
            Serial.println(F("Readings will update below:\n"));

            step = 5;
            break;
        }

        case 5:
            // Continuous reading mode after calibration
            loadCell.update();

            static unsigned long lastCalibDisplay = 0;
            if (millis() - lastCalibDisplay >= 500) {
                lastCalibDisplay = millis();

                float weight = loadCell.getAverageWeight(READINGS_TO_AVERAGE);
                Serial.print(F("Weight: "));
                Serial.print(weight, 2);
                Serial.print(F(" g"));

                if (loadCell.isStable()) {
                    Serial.print(F(" [STABLE]"));
                }
                Serial.println();
            }
            break;
    }
}

void displayWeight() {
    float weight = loadCell.getAverageWeight(READINGS_TO_AVERAGE);

    Serial.print(F("Weight: "));

    // Handle negative weights (might indicate reversed wiring)
    if (weight < -MIN_WEIGHT_THRESHOLD) {
        Serial.print(weight, 2);
        Serial.print(F(" g (negative - check wiring or tare)"));
    }
    // Filter out noise near zero
    else if (fabs(weight) < MIN_WEIGHT_THRESHOLD) {
        Serial.print(F("0.00 g"));
    }
    // Normal reading
    else {
        Serial.print(weight, 2);
        Serial.print(F(" g"));

        // Show in kg for larger weights
        if (weight >= 1000) {
            Serial.print(F(" ("));
            Serial.print(weight / 1000.0, 3);
            Serial.print(F(" kg)"));
        }
    }

    // Stability indicator
    if (loadCell.isStable()) {
        Serial.print(F(" [STABLE]"));
    }

    Serial.println();
}

void displayStabilityStatus() {
    Serial.println(F("----- Status -----"));
    Serial.print(F("HX711 Status: "));
    Serial.println(loadCell.getStatusString());
    Serial.print(F("Calibration: "));
    Serial.println(loadCell.getCalibrationFactor());
    Serial.println(F("------------------"));
    Serial.println();
}

void printSeparator() {
    Serial.println(F("===================================="));
}

void printWiringGuide() {
    Serial.println(F("=== Wiring Guide ==="));
    Serial.println(F("HX711       ESP32"));
    Serial.println(F("------      -----"));
    Serial.println(F("VCC    -->  3.3V"));
    Serial.println(F("GND    -->  GND"));
    Serial.print(F("DT     -->  GPIO"));
    Serial.println(LOADCELL_DOUT_PIN);
    Serial.print(F("SCK    -->  GPIO"));
    Serial.println(LOADCELL_SCK_PIN);
    Serial.println();
    Serial.println(F("Load Cell to HX711:"));
    Serial.println(F("  Red   (E+) -> E+"));
    Serial.println(F("  Black (E-) -> E-"));
    Serial.println(F("  White (A-) -> A-"));
    Serial.println(F("  Green (A+) -> A+"));
    Serial.println(F("(Wire colors may vary!)"));
    Serial.println();
}
