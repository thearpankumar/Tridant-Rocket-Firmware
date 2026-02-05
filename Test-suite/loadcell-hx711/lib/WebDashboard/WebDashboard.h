#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ThrustMetrics.h"
#include "wifi_config.h"

// Forward declaration for callback
class WebDashboard;
typedef void (*TareCallback)();
typedef void (*CalibrateCallback)(float weightGrams);

class WebDashboard {
public:
    WebDashboard();
    ~WebDashboard();

    // Initialization
    bool beginAP(const char* ssid = WIFI_AP_SSID, const char* password = WIFI_AP_PASSWORD);
    bool beginStation(const char* ssid, const char* password);

    // Data streaming
    void sendThrustData(float forceNewtons, unsigned long timestampMs);

    // Callbacks for commands
    void onTare(TareCallback callback) { _tareCallback = callback; }
    void onCalibrate(CalibrateCallback callback) { _calibrateCallback = callback; }

    // Session control
    void startRecording();
    void stopRecording();
    void resetSession();

    // Status
    bool isConnected() const;
    uint8_t getClientCount() const;
    String getIPAddress() const;

    // Metrics access
    ThrustMetrics& getMetrics() { return _metrics; }

private:
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    ThrustMetrics _metrics;

    // State
    bool _recording;
    bool _initialized;
    unsigned long _sessionStartTime;

    // Rate limiting
    unsigned long _lastDataSend;
    unsigned long _lastMetricsSend;
    uint8_t _dataDecimator;

    // Callbacks
    TareCallback _tareCallback;
    CalibrateCallback _calibrateCallback;

    // Internal methods
    void setupRoutes();
    void handleWebSocketMessage(AsyncWebSocketClient* client, const char* data);
    void sendMetrics();
    void cleanupClients();

    // WebSocket event handler (static for callback)
    static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);
    static WebDashboard* _instance;
};

#endif // WEB_DASHBOARD_H
