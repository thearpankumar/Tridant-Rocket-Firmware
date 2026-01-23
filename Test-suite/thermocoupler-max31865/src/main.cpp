#include <Arduino.h>
#include <math.h>
#include "RTDModule.h"
#include "rtd_config.h"

// ===== Global Objects =====
RTDModule rtdSensor;

// ===== Timing Variables =====
unsigned long lastDisplayTime = 0;
unsigned long lastStabilityTime = 0;

// ===== Function Prototypes =====
void displayTemperature();
void displayStatus();
void displayRawData();
void printSeparator();
void printWiringGuide();
void printHelp();
void checkAlertThresholds(float temp);

void setup() {
    // Initialize Serial Monitor
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    printSeparator();
    Serial.println(F("  MAX31865 RTD Temperature Sensor Test"));
    Serial.println(F("  ESP32 Dev Board"));
    printSeparator();
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.println();

    // Print wiring guide
    printWiringGuide();

    // Configure RTD type before initialization
    Serial.println(F("Initializing MAX31865..."));
    Serial.print(F("RTD Type: PT"));
    Serial.println(RTD_TYPE);
    Serial.print(F("Reference Resistor: "));
    Serial.print(RTD_RREF);
    Serial.println(F(" ohms"));
    Serial.print(F("Wire Configuration: "));
    Serial.print(RTD_WIRES);
    Serial.println(F("-wire"));
    Serial.println();

    Serial.println(F("SPI Pins:"));
    Serial.print(F("  CS:   GPIO"));
    Serial.println(MAX31865_CS_PIN);
    Serial.print(F("  MOSI: GPIO"));
    Serial.println(MAX31865_MOSI_PIN);
    Serial.print(F("  MISO: GPIO"));
    Serial.println(MAX31865_MISO_PIN);
    Serial.print(F("  CLK:  GPIO"));
    Serial.println(MAX31865_CLK_PIN);
    Serial.println();

    // Set RTD configuration
    rtdSensor.setRTDType(RTD_RNOMINAL, RTD_RREF);
    rtdSensor.setWireConfig(RTD_WIRES);

    // Initialize sensor with software SPI
    if (!rtdSensor.begin(MAX31865_CS_PIN, MAX31865_MOSI_PIN, MAX31865_MISO_PIN, MAX31865_CLK_PIN)) {
        Serial.println(F("\nFATAL: MAX31865 initialization failed!"));
        Serial.println(F("Check wiring:"));
        Serial.println(F("  - Is MAX31865 powered? (VIN -> 3.3V)"));
        Serial.println(F("  - Are SPI pins connected correctly?"));
        Serial.println(F("  - Is RTD sensor connected to the board?"));
        Serial.println(F("  - Check solder jumpers for wire configuration"));
        Serial.println(F("System halted."));
        while (1) {
            delay(1000);
        }
    }

    Serial.println(F("MAX31865 initialized successfully!"));

    // Check for initial faults
    if (rtdSensor.hasFault()) {
        Serial.println(F("WARNING: Fault detected!"));
        Serial.print(F("Fault: "));
        Serial.println(rtdSensor.getFaultString());
        Serial.println(F("Clearing fault..."));
        rtdSensor.clearFault();
    }

    // Configure stability detection
    rtdSensor.setStabilityThreshold(STABILITY_THRESHOLD);
    rtdSensor.setStabilitySamples(STABILITY_SAMPLES);

    Serial.println();
    Serial.println(F("Temperature alert thresholds:"));
    Serial.print(F("  Low:  "));
    Serial.print(TEMP_ALERT_LOW);
    Serial.println(F(" C"));
    Serial.print(F("  High: "));
    Serial.print(TEMP_ALERT_HIGH);
    Serial.println(F(" C"));

    printSeparator();
    Serial.println(F("Type 'h' for help"));
    Serial.println();
}

void loop() {
    // Update sensor data
    rtdSensor.update();

    unsigned long currentTime = millis();

    // Display temperature at regular intervals
    if (currentTime - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = currentTime;
        displayTemperature();
    }

    // Display status periodically
    if (currentTime - lastStabilityTime >= STABILITY_INTERVAL) {
        lastStabilityTime = currentTime;
        displayStatus();
    }

    // Check for serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'r':
            case 'R':
                displayRawData();
                break;

            case 'f':
            case 'F':
                Serial.println(F("\n--- Fault Status ---"));
                if (rtdSensor.hasFault()) {
                    Serial.print(F("Fault: "));
                    Serial.println(rtdSensor.getFaultString());
                    Serial.println(F("Type 'c' to clear fault"));
                } else {
                    Serial.println(F("No faults detected"));
                }
                Serial.println();
                break;

            case 'c':
            case 'C':
                Serial.println(F("\nClearing fault..."));
                rtdSensor.clearFault();
                Serial.println(F("Fault cleared!"));
                Serial.println();
                break;

            case 's':
            case 'S':
                displayStatus();
                break;

            case 'a':
            case 'A':
                Serial.println(F("\n--- Average Temperature ---"));
                Serial.print(F("Averaging "));
                Serial.print(READINGS_TO_AVERAGE);
                Serial.println(F(" readings..."));
                {
                    float avgTemp = rtdSensor.getAverageTemperature(READINGS_TO_AVERAGE);
                    Serial.print(F("Average Temperature: "));
                    Serial.print(avgTemp, 2);
                    Serial.print(F(" C ("));
                    Serial.print((avgTemp * 9.0 / 5.0) + 32.0, 2);
                    Serial.println(F(" F)"));
                }
                Serial.println();
                break;

            case 'h':
            case 'H':
            case '?':
                printHelp();
                break;
        }
    }
}

void displayTemperature() {
    RTDData data = rtdSensor.getData();

    Serial.print(F("Temp: "));

    if (!data.isValid) {
        Serial.print(F("FAULT - "));
        Serial.println(rtdSensor.getFaultString());
        return;
    }

    // Check for out of range values
    if (data.temperature < TEMP_MIN || data.temperature > TEMP_MAX) {
        Serial.print(data.temperature, 2);
        Serial.println(F(" C (OUT OF RANGE - check RTD connection)"));
        return;
    }

    // Display temperature in Celsius
    Serial.print(data.temperature, 2);
    Serial.print(F(" C"));

    // Display in Fahrenheit as well
    Serial.print(F(" ("));
    Serial.print((data.temperature * 9.0 / 5.0) + 32.0, 2);
    Serial.print(F(" F)"));

    // Stability indicator
    if (data.isStable) {
        Serial.print(F(" [STABLE]"));
    }

    Serial.println();

    // Check alert thresholds
    checkAlertThresholds(data.temperature);
}

void checkAlertThresholds(float temp) {
    static bool lowAlertActive = false;
    static bool highAlertActive = false;

    if (temp <= TEMP_ALERT_LOW && !lowAlertActive) {
        Serial.println(F("*** ALERT: Temperature below low threshold! ***"));
        lowAlertActive = true;
    } else if (temp > TEMP_ALERT_LOW) {
        lowAlertActive = false;
    }

    if (temp >= TEMP_ALERT_HIGH && !highAlertActive) {
        Serial.println(F("*** ALERT: Temperature above high threshold! ***"));
        highAlertActive = true;
    } else if (temp < TEMP_ALERT_HIGH) {
        highAlertActive = false;
    }
}

void displayRawData() {
    Serial.println(F("\n--- Raw RTD Data ---"));
    Serial.print(F("Raw ADC Value: "));
    Serial.println(rtdSensor.getRawRTD());
    Serial.print(F("RTD Resistance: "));
    Serial.print(rtdSensor.getResistance(), 2);
    Serial.println(F(" ohms"));
    Serial.print(F("Reference Resistor: "));
    Serial.print(RTD_RREF);
    Serial.println(F(" ohms"));
    Serial.print(F("RTD Nominal (0C): "));
    Serial.print(RTD_RNOMINAL);
    Serial.println(F(" ohms"));
    Serial.println();
}

void displayStatus() {
    Serial.println(F("----- Status -----"));
    Serial.print(F("Sensor Status: "));
    Serial.println(rtdSensor.getStatusString());

    RTDData data = rtdSensor.getData();
    Serial.print(F("Last Reading: "));
    Serial.print(data.temperature, 2);
    Serial.print(F(" C, Resistance: "));
    Serial.print(data.resistance, 2);
    Serial.println(F(" ohms"));

    if (rtdSensor.hasFault()) {
        Serial.print(F("Fault: "));
        Serial.println(rtdSensor.getFaultString());
    }

    Serial.println(F("------------------"));
    Serial.println();
}

void printSeparator() {
    Serial.println(F("===================================="));
}

void printWiringGuide() {
    Serial.println(F("=== Wiring Guide ==="));
    Serial.println(F("MAX31865     ESP32 (VSPI)"));
    Serial.println(F("--------     -----------"));
    Serial.println(F("VIN     -->  3.3V"));
    Serial.println(F("GND     -->  GND"));
    Serial.print(F("CLK     -->  GPIO"));
    Serial.println(MAX31865_CLK_PIN);
    Serial.print(F("SDO     -->  GPIO"));
    Serial.println(MAX31865_MISO_PIN);
    Serial.print(F("SDI     -->  GPIO"));
    Serial.println(MAX31865_MOSI_PIN);
    Serial.print(F("CS      -->  GPIO"));
    Serial.println(MAX31865_CS_PIN);
    Serial.println();

    Serial.println(F("RTD to MAX31865 (4-Wire):"));
    Serial.println(F("  Red wire 1  -> F+"));
    Serial.println(F("  Red wire 2  -> RTD+"));
    Serial.println(F("  Blue wire 1 -> F-"));
    Serial.println(F("  Blue wire 2 -> RTD-"));
    Serial.println();
}

void printHelp() {
    Serial.println(F("\n=== Commands ==="));
    Serial.println(F("r - Show raw ADC/resistance data"));
    Serial.println(F("a - Show averaged temperature"));
    Serial.println(F("f - Show fault status"));
    Serial.println(F("c - Clear fault"));
    Serial.println(F("s - Show sensor status"));
    Serial.println(F("h - Show this help"));
    Serial.println();
}
