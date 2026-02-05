#ifndef RTD_MODULE_H
#define RTD_MODULE_H

#include <Arduino.h>
#include <Adafruit_MAX31865.h>

// ===== RTD Data Structure =====
struct RTDData {
    float temperature;      // Temperature in Celsius
    float resistance;       // RTD resistance in ohms
    uint16_t rtdRaw;        // Raw RTD ADC value
    uint8_t fault;          // Fault status byte
    bool isStable;          // Temperature reading stability flag
    bool isValid;           // Data validity flag
    unsigned long timestamp; // Reading timestamp
};

// ===== RTD Module Class =====
class RTDModule {
public:
    RTDModule();
    ~RTDModule();

    // Initialization (Software SPI)
    bool begin(uint8_t csPin, uint8_t mosiPin, uint8_t misoPin, uint8_t clkPin);

    // Initialization (Hardware SPI)
    bool begin(uint8_t csPin);

    // Configuration
    void setRTDType(float nominalResistance, float referenceResistance);
    void setWireConfig(uint8_t wires);

    // Measurement
    void update();
    RTDData getData() const;
    float getTemperature();
    float getTemperatureFahrenheit();
    float getResistance();
    uint16_t getRawRTD();

    // Averaging
    float getAverageTemperature(uint8_t readings = 10);

    // Status
    bool isReady() const;
    bool isStable() const;
    bool hasFault();
    uint8_t getFault();
    void clearFault();
    const char* getStatusString();
    const char* getFaultString();

    // Stability detection
    void updateStability();
    void setStabilityThreshold(float threshold);
    void setStabilitySamples(uint8_t samples);

private:
    Adafruit_MAX31865* _rtd;
    float _nominalResistance;
    float _referenceResistance;
    uint8_t _wireConfig;
    bool _initialized;
    bool _useHardwareSPI;
    RTDData _currentData;

    // Stability tracking
    float _stabilityThreshold;
    uint8_t _stabilitySamples;
    float* _recentReadings;
    uint8_t _readingIndex;
    bool _isStable;

    // Internal helpers
    void updateCurrentData(float temp, float resistance, uint16_t rawRTD, uint8_t fault);
    float calculateStdDev() const;
    max31865_numwires_t getWireEnum() const;
};

#endif // RTD_MODULE_H
