#include "GPSModule.h"
#include "gps_config.h"

GPSModule::GPSModule() : gps(),
#if defined(USE_SOFTWARE_SERIAL)
    gpsSerial(nullptr),
    softSerial(nullptr),
#else
    gpsSerial(nullptr),
#endif
    initialized(false) {
}

bool GPSModule::begin(uint8_t rxPin, uint8_t txPin, uint32_t baud) {
#if defined(USE_SOFTWARE_SERIAL)
    // Use SoftwareSerial for Arduino Uno
    softSerial = new SoftwareSerial(rxPin, txPin);
    gpsSerial = softSerial;
    gpsSerial->begin(baud);
#else
    // Use Hardware Serial 2 for ESP32
    static HardwareSerial serialGPS(2);
    gpsSerial = &serialGPS;

    // Initialize GPS serial with specified pins
    gpsSerial->begin(baud, SERIAL_8N1, rxPin, txPin);
#endif

    initialized = true;
    return true;
}

bool GPSModule::update() {
    if (!initialized) {
#if defined(USE_SOFTWARE_SERIAL)
        if (gpsSerial == nullptr) {
#else
        if (gpsSerial == nullptr) {
#endif
            return false;
        }
    }

    bool newData = false;

    // Read all available data from GPS
#if defined(USE_SOFTWARE_SERIAL)
    while (gpsSerial->available() > 0) {
        char c = gpsSerial->read();
        if (gps.encode(c)) {
            newData = true;
        }
    }
#else
    while (gpsSerial->available() > 0) {
        char c = gpsSerial->read();
        if (gps.encode(c)) {
            newData = true;
        }
    }
#endif

    return newData;
}

GPSData GPSModule::getData() {
    GPSData data;

    // Location
    data.latitude = gps.location.lat();
    data.longitude = gps.location.lng();
    data.locationValid = gps.location.isValid();

    // Altitude
    data.altitude = gps.altitude.meters();
    data.altitudeValid = gps.altitude.isValid();

    // Speed
    data.speedKmph = gps.speed.kmph();
    data.speedMps = gps.speed.mps();
    data.speedValid = gps.speed.isValid();

    // Course
    data.course = gps.course.deg();
    data.courseValid = gps.course.isValid();

    // Satellites
    data.satellites = gps.satellites.value();
    data.satellitesValid = gps.satellites.isValid();

    // HDOP
    data.hdop = gps.hdop.hdop();
    data.hdopValid = gps.hdop.isValid();

    // Date
    data.year = gps.date.year();
    data.month = gps.date.month();
    data.day = gps.date.day();
    data.dateValid = gps.date.isValid();

    // Time
    data.hour = gps.time.hour();
    data.minute = gps.time.minute();
    data.second = gps.time.second();
    data.timeValid = gps.time.isValid();

    // Fix age
    data.fixAge = gps.location.age();

    return data;
}

double GPSModule::getLatitude() {
    return gps.location.lat();
}

double GPSModule::getLongitude() {
    return gps.location.lng();
}

double GPSModule::getAltitude() {
    return gps.altitude.meters();
}

double GPSModule::getSpeedKmph() {
    return gps.speed.kmph();
}

double GPSModule::getSpeedMps() {
    return gps.speed.mps();
}

double GPSModule::getCourse() {
    return gps.course.deg();
}

uint32_t GPSModule::getSatellites() {
    return gps.satellites.value();
}

double GPSModule::getHDOP() {
    return gps.hdop.hdop();
}

bool GPSModule::hasValidFix() {
    return gps.location.isValid() && gps.location.age() < 2000;
}

bool GPSModule::isLocationValid() {
    return gps.location.isValid();
}

bool GPSModule::isAltitudeValid() {
    return gps.altitude.isValid();
}

bool GPSModule::isSpeedValid() {
    return gps.speed.isValid();
}

bool GPSModule::isCourseValid() {
    return gps.course.isValid();
}

bool GPSModule::isTimeValid() {
    return gps.time.isValid();
}

bool GPSModule::isDateValid() {
    return gps.date.isValid();
}

const char* GPSModule::getFixQuality() {
    if (!gps.location.isValid()) {
        return "No Fix";
    }

    uint32_t sats = gps.satellites.value();
    if (sats >= MIN_SATELLITES_3D && gps.altitude.isValid()) {
        return "3D Fix";
    } else if (sats >= MIN_SATELLITES_2D) {
        return "2D Fix";
    }

    return "Poor Fix";
}

const char* GPSModule::getHDOPQuality() {
    if (!gps.hdop.isValid()) {
        return "Unknown";
    }

    double hdop = gps.hdop.hdop();

    if (hdop <= HDOP_IDEAL) {
        return "Ideal";
    } else if (hdop <= HDOP_EXCELLENT) {
        return "Excellent";
    } else if (hdop <= HDOP_GOOD) {
        return "Good";
    } else if (hdop <= HDOP_MODERATE) {
        return "Moderate";
    } else if (hdop <= HDOP_FAIR) {
        return "Fair";
    }

    return "Poor";
}

uint32_t GPSModule::getCharsProcessed() {
    return gps.charsProcessed();
}

uint32_t GPSModule::getSentencesWithFix() {
    return gps.sentencesWithFix();
}

uint32_t GPSModule::getFailedChecksums() {
    return gps.failedChecksum();
}

uint32_t GPSModule::getFixAge() {
    return gps.location.age();
}

void GPSModule::getDateTimeString(char* buffer) {
    if (gps.date.isValid() && gps.time.isValid()) {
        sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d UTC",
                gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        strcpy(buffer, "Date/Time Invalid");
    }
}

void GPSModule::getLocationString(char* buffer) {
    if (gps.location.isValid()) {
        sprintf(buffer, "%.6f, %.6f",
                gps.location.lat(), gps.location.lng());
    } else {
        strcpy(buffer, "Location Invalid");
    }
}

int GPSModule::available() {
    if (!initialized) {
#if defined(USE_SOFTWARE_SERIAL)
        if (gpsSerial == nullptr) {
#else
        if (gpsSerial == nullptr) {
#endif
            return 0;
        }
    }
    
#if defined(USE_SOFTWARE_SERIAL)
    return gpsSerial->available();
#else
    return gpsSerial->available();
#endif
}

char GPSModule::readRaw() {
    if (!initialized) {
#if defined(USE_SOFTWARE_SERIAL)
        if (gpsSerial == nullptr) {
            return 0;
        }
#else
        if (gpsSerial == nullptr) {
            return 0;
        }
#endif
    }
    
#if defined(USE_SOFTWARE_SERIAL)
    return gpsSerial->read();
#else
    return gpsSerial->read();
#endif
}