#include "WebDashboard.h"

// Static instance pointer for callback
WebDashboard* WebDashboard::_instance = nullptr;

WebDashboard::WebDashboard()
    : _server(nullptr)
    , _ws(nullptr)
    , _recording(false)
    , _initialized(false)
    , _sessionStartTime(0)
    , _lastDataSend(0)
    , _lastMetricsSend(0)
    , _dataDecimator(0)
    , _tareCallback(nullptr)
    , _calibrateCallback(nullptr)
{
    _instance = this;
}

WebDashboard::~WebDashboard() {
    if (_ws) {
        _ws->closeAll();
        delete _ws;
    }
    if (_server) {
        delete _server;
    }
    _instance = nullptr;
}

bool WebDashboard::beginAP(const char* ssid, const char* password) {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println(F("# Dashboard: LittleFS mount failed!"));
        return false;
    }
    Serial.println(F("# Dashboard: LittleFS mounted"));

    // Configure WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP_ADDR, AP_GATEWAY, AP_SUBNET);

    if (!WiFi.softAP(ssid, password, WIFI_AP_CHANNEL, false, WIFI_AP_MAX_CONNECTIONS)) {
        Serial.println(F("# Dashboard: AP setup failed!"));
        return false;
    }

    Serial.print(F("# Dashboard: AP started - SSID: "));
    Serial.println(ssid);
    Serial.print(F("# Dashboard: IP: "));
    Serial.println(WiFi.softAPIP());

    // Create server and websocket
    _server = new AsyncWebServer(WEB_SERVER_PORT);
    _ws = new AsyncWebSocket(WEBSOCKET_PATH);

    // Setup WebSocket handler
    _ws->onEvent(onWsEvent);
    _server->addHandler(_ws);

    // Setup routes
    setupRoutes();

    // Start server
    _server->begin();
    Serial.println(F("# Dashboard: Web server started"));

    _initialized = true;
    _sessionStartTime = millis();

    return true;
}

bool WebDashboard::beginStation(const char* ssid, const char* password) {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println(F("# Dashboard: LittleFS mount failed!"));
        return false;
    }

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print(F("# Dashboard: Connecting to "));
    Serial.println(ssid);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("\n# Dashboard: WiFi connection failed!"));
        return false;
    }

    Serial.println();
    Serial.print(F("# Dashboard: Connected - IP: "));
    Serial.println(WiFi.localIP());

    // Create server and websocket
    _server = new AsyncWebServer(WEB_SERVER_PORT);
    _ws = new AsyncWebSocket(WEBSOCKET_PATH);

    _ws->onEvent(onWsEvent);
    _server->addHandler(_ws);

    setupRoutes();
    _server->begin();

    _initialized = true;
    _sessionStartTime = millis();

    return true;
}

void WebDashboard::setupRoutes() {
    // Serve static files from LittleFS
    _server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // API endpoint for status
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["recording"] = _recording;
        doc["clients"] = _ws->count();
        doc["uptime"] = (millis() - _sessionStartTime) / 1000;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // 404 handler
    _server->onNotFound([](AsyncWebServerRequest* request) {
        request->send(404, "text/plain", "Not found");
    });
}

void WebDashboard::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                              AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (!_instance) return;

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("# Dashboard: Client #%u connected\n", client->id());
            // Send initial state
            {
                JsonDocument doc;
                doc["type"] = "init";
                doc["recording"] = _instance->_recording;
                String msg;
                serializeJson(doc, msg);
                client->text(msg);
            }
            break;

        case WS_EVT_DISCONNECT:
            Serial.printf("# Dashboard: Client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA:
            {
                AwsFrameInfo* info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    data[len] = 0; // Null terminate
                    _instance->handleWebSocketMessage(client, (char*)data);
                }
            }
            break;

        case WS_EVT_ERROR:
            Serial.printf("# Dashboard: WebSocket error on client #%u\n", client->id());
            break;

        case WS_EVT_PONG:
            break;
    }
}

void WebDashboard::handleWebSocketMessage(AsyncWebSocketClient* client, const char* data) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        Serial.println(F("# Dashboard: JSON parse error"));
        return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "tare") == 0) {
        // Send clear signal BEFORE tare (so client clears chart)
        _ws->textAll("{\"type\":\"clear\"}");
        _sessionStartTime = millis();
        _metrics.reset();
        if (_tareCallback) {
            _tareCallback();
        }
        _ws->textAll("{\"type\":\"ack\",\"cmd\":\"tare\"}");
    }
    else if (strcmp(cmd, "start") == 0) {
        startRecording();
        _ws->textAll("{\"type\":\"ack\",\"cmd\":\"start\"}");
    }
    else if (strcmp(cmd, "stop") == 0) {
        stopRecording();
        _ws->textAll("{\"type\":\"ack\",\"cmd\":\"stop\"}");
    }
    else if (strcmp(cmd, "reset") == 0) {
        // Send clear signal for chart reset
        _ws->textAll("{\"type\":\"clear\"}");
        resetSession();
        _ws->textAll("{\"type\":\"ack\",\"cmd\":\"reset\"}");
    }
    else if (strcmp(cmd, "calibrate") == 0) {
        float weight = doc["value"] | 0.0f;
        if (weight > 0 && _calibrateCallback) {
            _calibrateCallback(weight);
        }
        _ws->textAll("{\"type\":\"ack\",\"cmd\":\"calibrate\"}");
    }
}

void WebDashboard::sendThrustData(float forceNewtons, unsigned long timestampMs) {
    if (!_initialized || !_ws) return;

    // Update metrics (always, for accurate calculations)
    if (_recording) {
        _metrics.update(forceNewtons, timestampMs);
    }

    unsigned long now = millis();

    // Cleanup dead connections every 5 seconds
    if (now - _lastDataSend > 5000 && _dataDecimator == 0) {
        cleanupClients();
    }

    // Rate limit WebSocket to 20Hz (every 50ms) to prevent buffer overflow
    if (now - _lastDataSend >= 50) {
        _lastDataSend = now;

        if (_ws->count() > 0) {
            // Use compact format
            char buffer[64];
            unsigned long relativeTime = timestampMs - _sessionStartTime;
            snprintf(buffer, sizeof(buffer), "{\"type\":\"data\",\"t\":%lu,\"f\":%.3f}",
                     relativeTime, forceNewtons);
            _ws->textAll(buffer);
        }

        _dataDecimator++;
    }

    // Send metrics at lower rate (4Hz)
    if (now - _lastMetricsSend >= WS_METRICS_INTERVAL_MS) {
        _lastMetricsSend = now;
        sendMetrics();
    }
}

void WebDashboard::sendMetrics() {
    if (!_ws || _ws->count() == 0) return;

    JsonDocument doc;
    doc["type"] = "metrics";
    doc["peak"] = _metrics.getPeakThrust();
    doc["impulse"] = _metrics.getTotalImpulse();
    doc["burn"] = _metrics.getBurnTime();
    doc["avg"] = _metrics.getAverageThrust();
    doc["samples"] = _metrics.getSampleCount();
    doc["recording"] = _recording;

    String msg;
    serializeJson(doc, msg);
    _ws->textAll(msg);
}

void WebDashboard::startRecording() {
    _recording = true;
    _sessionStartTime = millis();
    _metrics.reset();
    Serial.println(F("# Dashboard: Recording started"));
}

void WebDashboard::stopRecording() {
    _recording = false;
    Serial.println(F("# Dashboard: Recording stopped"));
}

void WebDashboard::resetSession() {
    _recording = false;
    _sessionStartTime = millis();
    _metrics.reset();
    Serial.println(F("# Dashboard: Session reset"));
}

bool WebDashboard::isConnected() const {
    return _initialized && (_ws ? _ws->count() > 0 : false);
}

uint8_t WebDashboard::getClientCount() const {
    return _ws ? _ws->count() : 0;
}

String WebDashboard::getIPAddress() const {
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

void WebDashboard::cleanupClients() {
    if (_ws) {
        _ws->cleanupClients();
    }
}
