#include "RTDModule.h"
#include <math.h>
#include <new>

// ===== Constructor =====
RTDModule::RTDModule() :
    _rtd(nullptr),
    _nominalResistance(100.0),    // PT100 default
    _referenceResistance(430.0),  // PT100 reference resistor
    _wireConfig(3),               // 3-wire default
    _initialized(false),
    _useHardwareSPI(false),
    _stabilityThreshold(0.5),
    _stabilitySamples(5),
    _recentReadings(nullptr),
    _readingIndex(0),
    _isStable(false)
{
    _currentData = {0.0, 0.0, 0, 0, false, false, 0};
}

// ===== Destructor =====
RTDModule::~RTDModule() {
    if (_rtd != nullptr) {
        delete _rtd;
        _rtd = nullptr;
    }
    if (_recentReadings != nullptr) {
        delete[] _recentReadings;
        _recentReadings = nullptr;
    }
}

// ===== Initialization (Software SPI) =====
bool RTDModule::begin(uint8_t csPin, uint8_t mosiPin, uint8_t misoPin, uint8_t clkPin) {
    // Clean up if already initialized
    if (_rtd != nullptr) {
        delete _rtd;
    }

    // Create MAX31865 object with software SPI
    _rtd = new (std::nothrow) Adafruit_MAX31865(csPin, mosiPin, misoPin, clkPin);
    if (_rtd == nullptr) {
        Serial.println(F("ERROR: Memory allocation failed for MAX31865!"));
        return false;
    }

    _useHardwareSPI = false;

    // Initialize the sensor
    if (!_rtd->begin(getWireEnum())) {
        Serial.println(F("ERROR: MAX31865 initialization failed!"));
        delete _rtd;
        _rtd = nullptr;
        return false;
    }

    // Allocate stability tracking array
    if (_recentReadings != nullptr) {
        delete[] _recentReadings;
    }
    _recentReadings = new (std::nothrow) float[_stabilitySamples];
    if (_recentReadings == nullptr) {
        Serial.println(F("ERROR: Memory allocation failed for stability array!"));
        delete _rtd;
        _rtd = nullptr;
        return false;
    }
    for (uint8_t i = 0; i < _stabilitySamples; i++) {
        _recentReadings[i] = 0.0;
    }

    _initialized = true;

    // Do an initial read to verify communication
    uint16_t rtd = _rtd->readRTD();
    if (rtd == 0 && _rtd->readFault() != 0) {
        Serial.println(F("WARNING: Initial read returned fault - check wiring"));
    }

    return true;
}

// ===== Initialization (Hardware SPI) =====
bool RTDModule::begin(uint8_t csPin) {
    // Clean up if already initialized
    if (_rtd != nullptr) {
        delete _rtd;
    }

    // Create MAX31865 object with hardware SPI
    _rtd = new (std::nothrow) Adafruit_MAX31865(csPin);
    if (_rtd == nullptr) {
        Serial.println(F("ERROR: Memory allocation failed for MAX31865!"));
        return false;
    }

    _useHardwareSPI = true;

    // Initialize the sensor
    if (!_rtd->begin(getWireEnum())) {
        Serial.println(F("ERROR: MAX31865 initialization failed!"));
        delete _rtd;
        _rtd = nullptr;
        return false;
    }

    // Allocate stability tracking array
    if (_recentReadings != nullptr) {
        delete[] _recentReadings;
    }
    _recentReadings = new (std::nothrow) float[_stabilitySamples];
    if (_recentReadings == nullptr) {
        Serial.println(F("ERROR: Memory allocation failed for stability array!"));
        delete _rtd;
        _rtd = nullptr;
        return false;
    }
    for (uint8_t i = 0; i < _stabilitySamples; i++) {
        _recentReadings[i] = 0.0;
    }

    _initialized = true;
    return true;
}

// ===== Configuration =====
void RTDModule::setRTDType(float nominalResistance, float referenceResistance) {
    _nominalResistance = nominalResistance;
    _referenceResistance = referenceResistance;
}

void RTDModule::setWireConfig(uint8_t wires) {
    if (wires >= 2 && wires <= 4) {
        _wireConfig = wires;
    }
}

max31865_numwires_t RTDModule::getWireEnum() const {
    switch (_wireConfig) {
        case 2: return MAX31865_2WIRE;
        case 4: return MAX31865_4WIRE;
        case 3:
        default: return MAX31865_3WIRE;
    }
}

// ===== Measurement =====
void RTDModule::update() {
    if (!_initialized || _rtd == nullptr) return;

    uint16_t rtdRaw = _rtd->readRTD();
    float ratio = rtdRaw;
    ratio /= 32768;
    float resistance = _referenceResistance * ratio;
    float temperature = _rtd->temperature(_nominalResistance, _referenceResistance);
    uint8_t fault = _rtd->readFault();

    updateCurrentData(temperature, resistance, rtdRaw, fault);
    updateStability();
}

RTDData RTDModule::getData() const {
    return _currentData;
}

float RTDModule::getTemperature() {
    if (!_initialized || _rtd == nullptr) return 0.0;
    return _rtd->temperature(_nominalResistance, _referenceResistance);
}

float RTDModule::getTemperatureFahrenheit() {
    return (getTemperature() * 9.0 / 5.0) + 32.0;
}

float RTDModule::getResistance() {
    if (!_initialized || _rtd == nullptr) return 0.0;

    uint16_t rtdRaw = _rtd->readRTD();
    float ratio = rtdRaw;
    ratio /= 32768;
    return _referenceResistance * ratio;
}

uint16_t RTDModule::getRawRTD() {
    if (!_initialized || _rtd == nullptr) return 0;
    return _rtd->readRTD();
}

float RTDModule::getAverageTemperature(uint8_t readings) {
    if (!_initialized || _rtd == nullptr || readings == 0) return 0.0;

    float sum = 0.0;
    for (uint8_t i = 0; i < readings; i++) {
        sum += _rtd->temperature(_nominalResistance, _referenceResistance);
        delay(5);  // Small delay between readings
    }
    return sum / readings;
}

// ===== Status =====
bool RTDModule::isReady() const {
    return _initialized && _rtd != nullptr;
}

bool RTDModule::isStable() const {
    return _isStable;
}

bool RTDModule::hasFault() {
    if (!_initialized || _rtd == nullptr) return true;
    return _rtd->readFault() != 0;
}

uint8_t RTDModule::getFault() {
    if (!_initialized || _rtd == nullptr) return 0xFF;
    return _rtd->readFault();
}

void RTDModule::clearFault() {
    if (_initialized && _rtd != nullptr) {
        _rtd->clearFault();
    }
}

const char* RTDModule::getStatusString() {
    if (!_initialized) {
        return "Not Initialized";
    }
    if (_rtd == nullptr) {
        return "No Sensor";
    }
    if (hasFault()) {
        return "Fault Detected";
    }
    if (_isStable) {
        return "Stable";
    }
    return "Measuring";
}

const char* RTDModule::getFaultString() {
    if (!_initialized || _rtd == nullptr) {
        return "No Sensor";
    }

    uint8_t fault = _rtd->readFault();

    if (fault == 0) {
        return "No Fault";
    }
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
        return "RTD High Threshold";
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
        return "RTD Low Threshold";
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
        return "REFIN- > 0.85 x VBIAS";
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
        return "REFIN- < 0.85 x VBIAS (FORCE- open)";
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
        return "RTDIN- < 0.85 x VBIAS (FORCE- open)";
    }
    if (fault & MAX31865_FAULT_OVUV) {
        return "Overvoltage/Undervoltage";
    }

    return "Unknown Fault";
}

// ===== Stability Detection =====
void RTDModule::updateStability() {
    if (!_initialized || _recentReadings == nullptr) return;

    // Store current reading
    _recentReadings[_readingIndex] = _currentData.temperature;
    _readingIndex = (_readingIndex + 1) % _stabilitySamples;

    // Calculate standard deviation
    float stdDev = calculateStdDev();

    // Temperature is stable if std dev is below threshold
    _isStable = (stdDev < _stabilityThreshold);
    _currentData.isStable = _isStable;
}

void RTDModule::setStabilityThreshold(float threshold) {
    _stabilityThreshold = threshold;
}

void RTDModule::setStabilitySamples(uint8_t samples) {
    if (samples == _stabilitySamples || samples == 0) return;

    // Reallocate array
    float* newArray = new (std::nothrow) float[samples];
    if (newArray == nullptr) {
        // Keep existing array if allocation fails
        return;
    }

    if (_recentReadings != nullptr) {
        delete[] _recentReadings;
    }

    _stabilitySamples = samples;
    _recentReadings = newArray;
    for (uint8_t i = 0; i < samples; i++) {
        _recentReadings[i] = 0.0;
    }
    _readingIndex = 0;
}

// ===== Internal Helpers =====
void RTDModule::updateCurrentData(float temp, float resistance, uint16_t rawRTD, uint8_t fault) {
    _currentData.temperature = temp;
    _currentData.resistance = resistance;
    _currentData.rtdRaw = rawRTD;
    _currentData.fault = fault;
    _currentData.isValid = (fault == 0);
    _currentData.timestamp = millis();
}

float RTDModule::calculateStdDev() const {
    if (_recentReadings == nullptr) return 0.0;

    // Calculate mean
    float sum = 0.0;
    for (uint8_t i = 0; i < _stabilitySamples; i++) {
        sum += _recentReadings[i];
    }
    float mean = sum / _stabilitySamples;

    // Calculate variance
    float variance = 0.0;
    for (uint8_t i = 0; i < _stabilitySamples; i++) {
        float diff = _recentReadings[i] - mean;
        variance += diff * diff;
    }
    variance /= _stabilitySamples;

    return sqrt(variance);
}
