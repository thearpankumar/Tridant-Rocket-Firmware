#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>
#include <TinyGPS++.h>
#if defined(USE_SOFTWARE_SERIAL)
#include <SoftwareSerial.h>
#else
#include <HardwareSerial.h>
#endif

// GPS data structure for easy access
struct GPSData {
    // Location
    double latitude;
    double longitude;
    bool locationValid;

    // Altitude
    double altitude;        // meters
    bool altitudeValid;

    // Speed
    double speedKmph;       // km/h
    double speedMps;        // m/s
    bool speedValid;

    // Course/Heading
    double course;          // degrees
    bool courseValid;

    // Satellites
    uint32_t satellites;
    bool satellitesValid;

    // HDOP (accuracy indicator)
    double hdop;
    bool hdopValid;

    // Date/Time (UTC)
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool dateValid;
    bool timeValid;

    // Fix age (milliseconds since last valid fix)
    uint32_t fixAge;
};

class GPSModule {
public:
    GPSModule();

    // Initialize GPS module
    // rxPin: GPIO pin connected to GPS TX
    // txPin: GPIO pin connected to GPS RX
    // baud: GPS baud rate (default 9600)
    bool begin(uint8_t rxPin, uint8_t txPin, uint32_t baud = 9600);

    // Process incoming GPS data
    // Call this frequently in loop() to parse NMEA sentences
    // Returns true if new valid sentence was parsed
    bool update();

    // Get all GPS data in a structure
    GPSData getData();

    // Individual data accessors
    double getLatitude();
    double getLongitude();
    double getAltitude();
    double getSpeedKmph();
    double getSpeedMps();
    double getCourse();
    uint32_t getSatellites();
    double getHDOP();

    // Check if we have a valid GPS fix
    bool hasValidFix();

    // Check if specific data is valid
    bool isLocationValid();
    bool isAltitudeValid();
    bool isSpeedValid();
    bool isCourseValid();
    bool isTimeValid();
    bool isDateValid();

    // Get fix quality description
    const char* getFixQuality();

    // Get HDOP quality description
    const char* getHDOPQuality();

    // Debug/diagnostic information
    uint32_t getCharsProcessed();
    uint32_t getSentencesWithFix();
    uint32_t getFailedChecksums();

    // Get time since last valid fix (milliseconds)
    uint32_t getFixAge();

    // Format date/time as string (buffer must be at least 20 chars)
    void getDateTimeString(char* buffer);

    // Format location as string (buffer must be at least 30 chars)
    void getLocationString(char* buffer);

    // Raw data access for debugging
    int available();
    char readRaw();

private:
    TinyGPSPlus gps;
#if defined(USE_SOFTWARE_SERIAL)
    SoftwareSerial* gpsSerial;
    SoftwareSerial* softSerial;
#else
    HardwareSerial* gpsSerial;
#endif
    bool initialized;
};

#endif // GPS_MODULE_H
