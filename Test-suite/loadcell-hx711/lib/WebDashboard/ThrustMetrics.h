#ifndef THRUST_METRICS_H
#define THRUST_METRICS_H

#include <Arduino.h>

// ============================================================================
// Thrust Metrics Calculator
// ============================================================================
// Real-time computation of thrust curve metrics for rocket motor testing

class ThrustMetrics {
public:
    ThrustMetrics() { reset(); }

    void reset() {
        _peakThrust = 0.0f;
        _totalImpulse = 0.0f;
        _thrustSum = 0.0f;
        _sampleCount = 0;
        _burnSampleCount = 0;
        _burnStartTime = 0;
        _burnEndTime = 0;
        _burnActive = false;
        _lastTimestamp = 0;
        _lastForce = 0.0f;
    }

    // Update metrics with new data point
    void update(float forceNewtons, unsigned long timestampMs) {
        // Skip invalid readings
        if (isnan(forceNewtons)) return;

        float absForce = fabs(forceNewtons);

        // Update peak thrust
        if (absForce > _peakThrust) {
            _peakThrust = absForce;
        }

        // Integrate impulse using trapezoidal rule
        if (_sampleCount > 0 && _lastTimestamp > 0) {
            float dt = (timestampMs - _lastTimestamp) / 1000.0f; // Convert to seconds
            if (dt > 0 && dt < 0.1f) { // Sanity check (ignore gaps > 100ms)
                float avgForce = (absForce + fabs(_lastForce)) / 2.0f;
                _totalImpulse += avgForce * dt;
            }
        }

        // Burn detection (threshold = 5% of peak or minimum threshold)
        float burnThreshold = max(_peakThrust * 0.05f, 0.1f);

        if (absForce >= burnThreshold) {
            if (!_burnActive) {
                _burnStartTime = timestampMs;
                _burnActive = true;
            }
            _burnEndTime = timestampMs;
            _thrustSum += absForce;
            _burnSampleCount++;
        }

        _lastTimestamp = timestampMs;
        _lastForce = forceNewtons;
        _sampleCount++;
    }

    // Getters
    float getPeakThrust() const { return _peakThrust; }
    float getTotalImpulse() const { return _totalImpulse; }

    float getBurnTime() const {
        if (_burnStartTime == 0 || _burnEndTime <= _burnStartTime) return 0.0f;
        return (_burnEndTime - _burnStartTime) / 1000.0f; // Return in seconds
    }

    float getAverageThrust() const {
        // Use actual burn sample count for accurate average
        if (_burnSampleCount == 0) return 0.0f;
        return _thrustSum / (float)_burnSampleCount;
    }

    uint32_t getSampleCount() const { return _sampleCount; }
    uint32_t getBurnSampleCount() const { return _burnSampleCount; }

    bool isBurnActive() const { return _burnActive; }

private:
    float _peakThrust;
    float _totalImpulse;
    float _thrustSum;
    uint32_t _sampleCount;
    uint32_t _burnSampleCount;
    unsigned long _burnStartTime;
    unsigned long _burnEndTime;
    bool _burnActive;
    unsigned long _lastTimestamp;
    float _lastForce;
};

#endif // THRUST_METRICS_H
