#include "LoadCellModule.h"
#include "loadcell_config.h"

// ===== Constructor =====
LoadCellModule::LoadCellModule() :
    _calibrationFactor(1.0f),
    _initialized(false),
    _tareOffset(0)
{
}

// ===== Initialization =====
bool LoadCellModule::begin(uint8_t doutPin, uint8_t sckPin) {
    _scale.begin(doutPin, sckPin);

    // Wait for HX711 to be ready (timeout 3s)
    unsigned long startTime = millis();
    while (!_scale.is_ready()) {
        if (millis() - startTime > 3000) {
            Serial.println(F("ERROR: HX711 not responding!"));
            return false;
        }
        delay(10);
    }

    _initialized = true;
    return true;
}

bool LoadCellModule::begin(uint8_t doutPin, uint8_t sckPin, float calibrationFactor) {
    if (!begin(doutPin, sckPin)) {
        return false;
    }
    setCalibrationFactor(calibrationFactor);
    return true;
}

// ===== Calibration =====
void LoadCellModule::setCalibrationFactor(float factor) {
    // Prevent division by zero - use 1.0 as safe default
    if (factor == 0.0f) {
        factor = 1.0f;
    }
    _calibrationFactor = factor;
}

float LoadCellModule::getCalibrationFactor() const {
    return _calibrationFactor;
}

void LoadCellModule::tare(uint8_t readings) {
    if (!_initialized || readings == 0) return;

    // Calculate tare offset as average of readings
    long sum = 0;
    for (uint8_t i = 0; i < readings; i++) {
        while (!_scale.is_ready()) {
            delay(1);
        }
        sum += _scale.read();
    }
    _tareOffset = sum / readings;
}

// ===== High-Speed Measurement =====
bool LoadCellModule::isReady() {
    return _initialized && _scale.is_ready();
}

ThrustData LoadCellModule::read() {
    ThrustData data;
    data.timestamp = millis();

    if (!_initialized) {
        data.forceNewtons = 0.0f;
        data.rawValue = 0;
        data.valid = false;
        return data;
    }

    // Blocking read
    data.rawValue = _scale.read();

    // Convert to Newtons: (raw - offset) / cal_factor
    // Calibration factor is in raw_units/Newton
    float rawCorrected = (float)(data.rawValue - _tareOffset);
    // Safety check for division (should never happen, but defensive)
    if (_calibrationFactor != 0.0f) {
        data.forceNewtons = rawCorrected / _calibrationFactor;
    } else {
        data.forceNewtons = 0.0f;
    }
    data.valid = true;

    return data;
}

bool LoadCellModule::readIfReady(ThrustData& data) {
    if (!isReady()) {
        return false;
    }
    data = read();
    return true;
}

float LoadCellModule::getForceNewtons() {
    if (!_initialized || _calibrationFactor == 0.0f) return 0.0f;

    long raw = _scale.read();
    float rawCorrected = (float)(raw - _tareOffset);
    return rawCorrected / _calibrationFactor;
}

long LoadCellModule::getRawValue() {
    if (!_initialized) return 0;
    return _scale.read();
}

long LoadCellModule::getAverageRawValue(uint8_t readings) {
    if (!_initialized || readings == 0) return 0;
    return _scale.read_average(readings);
}

// ===== Status =====
const char* LoadCellModule::getStatusString() {
    if (!_initialized) {
        return "Not Initialized";
    }
    return "Ready";
}

// ===== Power Management =====
void LoadCellModule::powerDown() {
    if (_initialized) {
        _scale.power_down();
    }
}

void LoadCellModule::powerUp() {
    if (_initialized) {
        _scale.power_up();
    }
}
