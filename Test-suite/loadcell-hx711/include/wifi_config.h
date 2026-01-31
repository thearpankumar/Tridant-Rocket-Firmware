#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// ============================================================================
// WiFi & Web Dashboard Configuration
// ============================================================================

// ===== WiFi Access Point Settings =====
#define WIFI_AP_SSID "ThrustTest"
#define WIFI_AP_PASSWORD "rocket123"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4

// ===== Web Server Settings =====
#define WEB_SERVER_PORT 80
#define WEBSOCKET_PATH "/ws"

// ===== WebSocket Timing =====
// Data streaming rate - full 80Hz for accuracy
#define WS_DATA_RATE_HZ 80
#define WS_DATA_INTERVAL_MS (1000 / WS_DATA_RATE_HZ)

// Metrics update rate
#define WS_METRICS_RATE_HZ 4
#define WS_METRICS_INTERVAL_MS (1000 / WS_METRICS_RATE_HZ)

// ===== Dashboard Features =====
// Burn detection threshold (percentage of peak)
#define BURN_THRESHOLD_PERCENT 5.0f

// Minimum force to consider as "active" (Newtons)
#define MIN_ACTIVE_FORCE_N 0.1f

// ===== IP Address (AP Mode) =====
// Default: 192.168.4.1
#define AP_IP_ADDR IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

#endif // WIFI_CONFIG_H
