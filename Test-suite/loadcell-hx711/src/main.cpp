#include "LoadCellModule.h"
#include "loadcell_config.h"
#include <Arduino.h>

// ===== Global Objects =====
LoadCellModule loadCell;

// ===== Timing =====
unsigned long startTime = 0;
bool outputEnabled = true;

// ===== Function Prototypes =====
void printHelp();
void handleSerialCommands();

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    // Minimal startup banner
    Serial.println();
    Serial.println(F("# =========================================="));
    Serial.println(F("# HX711 Thrust Test - 80Hz High-Speed Mode"));
    Serial.println(F("# =========================================="));
    Serial.print(F("# Board: "));
    Serial.println(BOARD_NAME);
    Serial.print(F("# Calibration Factor: "));
    Serial.println(CALIBRATION_FACTOR, 3);
    Serial.println(F("#"));
    Serial.println(F("# IMPORTANT: Ensure HX711 RATE pin is HIGH for 80Hz!"));
    Serial.println(F("#"));

    // Initialize load cell
    Serial.println(F("# Initializing HX711..."));

    if (!loadCell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, CALIBRATION_FACTOR)) {
        Serial.println(F("# FATAL: HX711 initialization failed!"));
        Serial.println(F("# Check wiring:"));
        Serial.println(F("#   HX711 VCC  -> 3.3V"));
        Serial.println(F("#   HX711 GND  -> GND"));
        Serial.print(F("#   HX711 DT   -> GPIO"));
        Serial.println(LOADCELL_DOUT_PIN);
        Serial.print(F("#   HX711 SCK  -> GPIO"));
        Serial.println(LOADCELL_SCK_PIN);
        Serial.println(F("# System halted."));
        while (1) {
            delay(1000);
        }
    }

    Serial.println(F("# HX711 OK"));
    Serial.println(F("#"));
    Serial.println(F("# Taring... ensure NO load on sensor!"));
    delay(1000);
    loadCell.tare(TARE_READINGS);
    Serial.println(F("# Tare complete."));
    Serial.println(F("#"));

    printHelp();

    Serial.println(F("#"));
    Serial.println(F("# Starting data output..."));
    Serial.println(F("# timestamp_ms,force_N"));

    startTime = millis();
}

void loop() {
    // Handle serial commands (non-blocking)
    handleSerialCommands();

    // Read and output at maximum rate (80Hz = ~12.5ms per sample)
    if (outputEnabled) {
        ThrustData data;
        if (loadCell.readIfReady(data) && data.valid) {
            // CSV output: timestamp_ms,force_N
            unsigned long relativeTime = data.timestamp - startTime;
            Serial.print(relativeTime);
            Serial.print(',');
            Serial.println(data.forceNewtons, 3);
        }
    }
}

void handleSerialCommands() {
    if (!Serial.available()) return;

    char cmd = Serial.read();

    switch (cmd) {
        case 't':
        case 'T':
            outputEnabled = false;
            Serial.println(F("# Taring... remove all load!"));
            delay(500);
            loadCell.tare(TARE_READINGS);
            Serial.println(F("# Tare complete."));
            startTime = millis();  // Reset timestamp
            Serial.println(F("# timestamp_ms,force_N"));
            outputEnabled = true;
            break;

        case 'r':
        case 'R':
            outputEnabled = false;
            Serial.println(F("# --- Raw ADC Reading ---"));
            Serial.print(F("# Raw Value: "));
            Serial.println(loadCell.getRawValue());
            Serial.print(F("# Avg Raw (10): "));
            Serial.println(loadCell.getAverageRawValue(10));
            Serial.println(F("# timestamp_ms,force_N"));
            outputEnabled = true;
            break;

        case 'p':
        case 'P':
            outputEnabled = !outputEnabled;
            if (outputEnabled) {
                Serial.println(F("# Output RESUMED"));
                Serial.println(F("# timestamp_ms,force_N"));
            } else {
                Serial.println(F("# Output PAUSED (press 'p' to resume)"));
            }
            break;

        case 'z':
        case 'Z':
            // Zero/reset timestamp
            startTime = millis();
            Serial.println(F("# Timestamp reset to 0"));
            Serial.println(F("# timestamp_ms,force_N"));
            break;

        case 'c':
        case 'C': {
            // Calibration helper
            outputEnabled = false;
            Serial.println(F("#"));
            Serial.println(F("# === CALIBRATION MODE ==="));
            Serial.println(F("#"));
            Serial.println(F("# Step 1: Remove all weight from load cell"));
            Serial.println(F("# Press ENTER when ready..."));

            // Wait for Enter key
            while (!Serial.available()) delay(10);
            while (Serial.available()) Serial.read();

            Serial.println(F("# Reading zero point (20 samples)..."));
            delay(500);  // Allow sensor to settle
            long rawZero = loadCell.getAverageRawValue(20);
            Serial.print(F("# Raw (no weight): "));
            Serial.println(rawZero);

            Serial.println(F("#"));
            Serial.println(F("# Step 2: Place known weight on load cell"));
            Serial.println(F("# Then enter the weight in GRAMS (e.g., 500 or 1000):"));

            // Read weight input from serial
            char inputBuffer[16];
            uint8_t inputIndex = 0;
            memset(inputBuffer, 0, sizeof(inputBuffer));

            // Wait for and read numeric input
            while (true) {
                if (Serial.available()) {
                    char c = Serial.read();
                    // Enter key ends input
                    if (c == '\r' || c == '\n') {
                        if (inputIndex > 0) break;  // Only break if we have input
                    }
                    // Accept digits and decimal point
                    else if ((c >= '0' && c <= '9') || c == '.') {
                        if (inputIndex < sizeof(inputBuffer) - 1) {
                            inputBuffer[inputIndex++] = c;
                            Serial.print(c);  // Echo character
                        }
                    }
                }
                delay(10);
            }
            Serial.println();  // New line after input

            // Parse input
            float weightGrams = atof(inputBuffer);

            // Validate input
            if (weightGrams <= 0) {
                Serial.println(F("# ERROR: Invalid weight! Must be > 0"));
                Serial.println(F("# Calibration aborted."));
                Serial.println(F("# timestamp_ms,force_N"));
                outputEnabled = true;
                break;
            }

            // Convert grams to Newtons
            float weightNewtons = weightGrams * GRAMS_TO_NEWTONS;

            Serial.print(F("# Weight entered: "));
            Serial.print(weightGrams, 1);
            Serial.print(F(" g = "));
            Serial.print(weightNewtons, 4);
            Serial.println(F(" N"));

            Serial.println(F("#"));
            Serial.println(F("# Reading with weight (20 samples)..."));
            delay(500);  // Allow sensor to settle
            long rawWeight = loadCell.getAverageRawValue(20);
            Serial.print(F("# Raw (with weight): "));
            Serial.println(rawWeight);

            long rawDiff = rawWeight - rawZero;
            Serial.print(F("# Raw difference: "));
            Serial.println(rawDiff);

            // Check for valid difference
            if (rawDiff == 0) {
                Serial.println(F("# ERROR: No change detected!"));
                Serial.println(F("# Check: Is weight actually on the sensor?"));
                Serial.println(F("# Check: Are wires connected properly?"));
                Serial.println(F("# Calibration aborted."));
                Serial.println(F("# timestamp_ms,force_N"));
                outputEnabled = true;
                break;
            }

            // Handle inverted load cell (negative difference)
            if (rawDiff < 0) {
                Serial.println(F("# NOTE: Negative difference detected"));
                Serial.println(F("# Load cell may be mounted inverted (compression mode)"));
                Serial.println(F("# Using absolute value for calibration"));
                rawDiff = -rawDiff;
            }

            // Calculate calibration factor
            float newCalFactor = (float)rawDiff / weightNewtons;

            Serial.println(F("#"));
            Serial.println(F("# === CALIBRATION RESULT ==="));
            Serial.print(F("# New calibration factor: "));
            Serial.println(newCalFactor, 3);
            Serial.println(F("#"));
            Serial.println(F("# To apply, update platformio.ini:"));
            Serial.print(F("#   -D CALIBRATION_FACTOR="));
            Serial.println(newCalFactor, 1);
            Serial.println(F("#"));
            Serial.println(F("# Or press 'a' now to apply temporarily"));
            Serial.println(F("# (will reset on power cycle)"));

            // Wait for response
            unsigned long waitStart = millis();
            bool applied = false;
            while (millis() - waitStart < 5000) {  // 5 second timeout
                if (Serial.available()) {
                    char response = Serial.read();
                    if (response == 'a' || response == 'A') {
                        loadCell.setCalibrationFactor(newCalFactor);
                        loadCell.tare(TARE_READINGS);
                        Serial.println(F("# Calibration APPLIED and tared!"));
                        Serial.print(F("# Active cal factor: "));
                        Serial.println(loadCell.getCalibrationFactor(), 3);
                        applied = true;
                        break;
                    } else if (response == '\r' || response == '\n') {
                        break;
                    }
                }
                delay(10);
            }

            if (!applied) {
                Serial.println(F("# Calibration NOT applied (update platformio.ini manually)"));
            }

            Serial.println(F("# === END CALIBRATION ==="));
            Serial.println(F("#"));
            Serial.println(F("# timestamp_ms,force_N"));
            outputEnabled = true;
            break;
        }

        case 'h':
        case 'H':
        case '?':
            outputEnabled = false;
            printHelp();
            Serial.println(F("# timestamp_ms,force_N"));
            outputEnabled = true;
            break;

        case '\r':
        case '\n':
            // Ignore newlines
            break;

        default:
            // Unknown command - ignore
            break;
    }
}

void printHelp() {
    Serial.println(F("# === Commands ==="));
    Serial.println(F("# t - Tare (zero) the sensor"));
    Serial.println(F("# r - Show raw ADC reading"));
    Serial.println(F("# p - Pause/resume output"));
    Serial.println(F("# z - Zero timestamp"));
    Serial.println(F("# c - Calibration mode (input weight in grams)"));
    Serial.println(F("# h - Show this help"));
}
