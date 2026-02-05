#ifndef LOADCELL_MODULE_H
#define LOADCELL_MODULE_H

#include <Arduino.h>
#include <HX711.h>

// ===== Thrust Data Structure =====
struct ThrustData {
    float forceNewtons;      // Force in Newtons (+ tension, - compression)
    long rawValue;           // Raw ADC value
    unsigned long timestamp; // Reading timestamp in ms
    bool valid;              // Data validity flag
};

// ===== Load Cell Module Class =====
// Optimized for high-speed thrust measurement
class LoadCellModule {
public:
    LoadCellModule();

    // Initialization
    bool begin(uint8_t doutPin, uint8_t sckPin);
    bool begin(uint8_t doutPin, uint8_t sckPin, float calibrationFactor);

    // Calibration
    void setCalibrationFactor(float factor);
    float getCalibrationFactor() const;
    void tare(uint8_t readings = 20);

    // High-speed measurement (non-blocking)
    bool isReady();
    ThrustData read();              // Blocking read
    bool readIfReady(ThrustData& data);  // Non-blocking read

    // Direct access methods
    float getForceNewtons();        // Single blocking read in Newtons
    long getRawValue();             // Single raw ADC read
    long getAverageRawValue(uint8_t readings);  // For calibration only

    // Status
    const char* getStatusString();

    // Power management
    void powerDown();
    void powerUp();

private:
    HX711 _scale;
    float _calibrationFactor;
    bool _initialized;
    long _tareOffset;
};

#endif // LOADCELL_MODULE_H
