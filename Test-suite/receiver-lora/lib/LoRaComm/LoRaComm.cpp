#include "LoRaComm.h"
#include <SPI.h>

LoRaComm::LoRaComm() : lastRSSI(0), lastSNR(0.0), radioModule(nullptr), radio(nullptr), initialized(false) {
}

LoRaComm::~LoRaComm() {
    if (radio != nullptr) {
        delete radio;
        radio = nullptr;
    }
    if (radioModule != nullptr) {
        delete radioModule;
        radioModule = nullptr;
    }
}

bool LoRaComm::begin() {
    Serial.println(F("\n=== LoRa Initialization Debug ==="));

    // Print module type
    #if defined(LORA_MODULE_SX1262)
        Serial.println(F("Module Type: SX1262"));
    #else
        Serial.println(F("Module Type: Ra-02 (SX1278)"));
    #endif

    // Print pin configuration
    Serial.print(F("NSS (CS): GPIO "));
    Serial.println(LORA_NSS);
    Serial.print(F("RESET: GPIO "));
    Serial.println(LORA_RESET);

    #if defined(LORA_MODULE_SX1262)
        Serial.print(F("DIO1: GPIO "));
        Serial.println(LORA_DIO1);
        Serial.print(F("BUSY: GPIO "));
        Serial.println(LORA_BUSY);
    #else
        Serial.print(F("DIO0: GPIO "));
        Serial.println(LORA_DIO0);
    #endif

    Serial.print(F("Frequency: "));
    Serial.print(LORA_FREQUENCY / 1E6);
    Serial.println(F(" MHz"));

    // Initialize SPI bus
    SPI.begin();
    delay(10);

    // Create RadioLib Module instance with appropriate pins
    #if defined(LORA_MODULE_SX1262)
        // SX1262: NSS, DIO1, RESET, BUSY
        radioModule = new Module(LORA_NSS, LORA_DIO1, LORA_RESET, LORA_BUSY);
    #else
        // SX1278 (Ra-02): NSS, DIO0, RESET, DIO1 (not used, set to NC)
        radioModule = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, RADIOLIB_NC);
    #endif

    // Create radio instance
    radio = new LoRaModuleType(radioModule);

    // Initialize the radio module
    Serial.println(F("Initializing radio module..."));

    // RadioLib begin() takes frequency in MHz
    int state = radio->begin(LORA_FREQUENCY / 1E6);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("\n!!! ERROR: LoRa initialization failed with code: "));
        Serial.println(state);
        Serial.println(F("\nPossible causes:"));
        Serial.println(F("  1. Wiring issues:"));
        Serial.println(F("     - Check SPI connections (NSS, MOSI, MISO, SCK)"));
        Serial.println(F("     - Verify GND connection"));
        Serial.println(F("     - Verify 3.3V power (NOT 5V!)"));
        Serial.println(F("  2. Module issues:"));
        Serial.println(F("     - LoRa module not powered"));
        #if defined(LORA_MODULE_SX1262)
            Serial.println(F("     - Damaged SX1262 chip"));
            Serial.println(F("     - Wrong module (not SX1262)"));
            Serial.println(F("     - Check BUSY pin connection"));
        #else
            Serial.println(F("     - Damaged SX1278 chip"));
            Serial.println(F("     - Wrong module (not Ra-02/SX1278)"));
        #endif
        Serial.println(F("  3. SPI bus conflict:"));
        Serial.println(F("     - Another device using same pins"));
        Serial.println(F("     - Check if pins are already in use"));
        Serial.println(F("  4. Pin configuration:"));
        Serial.println(F("     - Verify GPIO numbers match your wiring"));
        Serial.println(F("     - ESP32: GPIO34-39 are INPUT ONLY"));
        Serial.println(F("\nDouble-check your wiring against the pin numbers above!"));
        return false;
    }

    Serial.println(F("Radio module initialized successfully!"));

    #if defined(LORA_MODULE_SX1262)
        // SX1262-specific configuration

        // Set TCXO voltage with 5ms timeout - most SX1262 modules use TCXO
        // Common voltages: 1.6V, 1.7V, 1.8V, 2.4V, 2.7V, 3.0V, 3.3V
        // Timeout in microseconds (5000us = 5ms) for TCXO to stabilize
        // If your module doesn't have TCXO, this will fail - that's OK
        state = radio->setTCXO(1.6, 5000);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("TCXO configured at 1.6V"));
        } else {
            Serial.print(F("NOTE: TCXO not available (code: "));
            Serial.print(state);
            Serial.println(F(") - using crystal oscillator"));
        }

        // Use DC-DC regulator (more efficient, most modules use this)
        // If your module uses LDO, change to setRegulatorLDO()
        state = radio->setRegulatorDCDC();
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("WARNING: Failed to set DC-DC regulator, code: "));
            Serial.println(state);
        }

        // Set DIO2 as RF switch control - most SX1262 modules need this
        state = radio->setDio2AsRfSwitch(true);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("WARNING: Failed to set DIO2 as RF switch, code: "));
            Serial.println(state);
        }

        // Set current limit (default is 60mA, max is 140mA for SX1262)
        state = radio->setCurrentLimit(140.0);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("WARNING: Failed to set current limit, code: "));
            Serial.println(state);
        }
    #endif

    // Configure LoRa parameters
    state = radio->setSpreadingFactor(LORA_SPREADING_FACTOR);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to set spreading factor, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setBandwidth(LORA_SIGNAL_BANDWIDTH / 1E3);  // RadioLib uses kHz
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to set bandwidth, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setCodingRate(LORA_CODING_RATE);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to set coding rate, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setPreambleLength(LORA_PREAMBLE_LENGTH);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to set preamble length, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setSyncWord(LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to set sync word, code: "));
        Serial.println(state);
        return false;
    }

    #if defined(LORA_MODULE_SX1262)
        // SX1262: Use lower power if high power PA fails
        // Low-power PA: -17 to +14 dBm, High-power PA: -9 to +22 dBm
        state = radio->setOutputPower(LORA_TX_POWER);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("WARNING: TX power "));
            Serial.print(LORA_TX_POWER);
            Serial.println(F(" dBm failed, trying 14 dBm"));
            state = radio->setOutputPower(14);
            if (state != RADIOLIB_ERR_NONE) {
                Serial.print(F("ERROR: Failed to set TX power, code: "));
                Serial.println(state);
                return false;
            }
        }
    #else
        state = radio->setOutputPower(LORA_TX_POWER);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("ERROR: Failed to set TX power, code: "));
            Serial.println(state);
            return false;
        }
    #endif

    // Enable CRC
    state = radio->setCRC(true);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Failed to enable CRC, code: "));
        Serial.println(state);
        return false;
    }

    initialized = true;
    Serial.println(F("SUCCESS: LoRa module initialized"));
    printConfig();

    return true;
}

bool LoRaComm::sendPacket(const uint8_t* data, size_t length) {
    if (!initialized || radio == nullptr) {
        Serial.println(F("ERROR: LoRa not initialized"));
        return false;
    }

    if (length == 0 || length > 255) {
        Serial.println(F("ERROR: Invalid packet length"));
        return false;
    }

    // Transmit the packet (blocking)
    int state = radio->transmit((uint8_t*)data, length);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Packet transmission failed, code: "));
        Serial.println(state);
        return false;
    }

    return true;
}

int LoRaComm::receivePacket(uint8_t* buffer, size_t maxLength) {
    if (!initialized || radio == nullptr) {
        return 0;
    }

    // Try to receive a packet (non-blocking check using receive with timeout=0)
    // Note: For true non-blocking, you'd use startReceive() and interrupts
    // This implementation uses a short timeout for polling
    int state = radio->receive(buffer, maxLength);

    if (state == RADIOLIB_ERR_NONE) {
        // Packet received successfully
        lastRSSI = radio->getRSSI();
        lastSNR = radio->getSNR();
        return radio->getPacketLength();
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        // No packet available
        return 0;
    } else {
        // Some other error
        return 0;
    }
}

bool LoRaComm::isPacketAvailable() {
    // For RadioLib, we'd typically use interrupt-based reception
    // This polling approach isn't ideal but maintains API compatibility
    return false;
}

int LoRaComm::getRSSI() {
    return lastRSSI;
}

float LoRaComm::getSNR() {
    return lastSNR;
}

bool LoRaComm::isTransmitting() {
    // RadioLib uses blocking transmit by default
    return false;
}

void LoRaComm::onReceive(void (*callback)(int)) {
    // Note: RadioLib uses a different interrupt mechanism
    // This would require refactoring to use setDio0Action/setDio1Action
    // For now, this is a placeholder to maintain API compatibility
    (void)callback;  // Suppress unused parameter warning
}

void LoRaComm::printConfig() {
    Serial.println(F("--- LoRa Configuration ---"));

    #if defined(LORA_MODULE_SX1262)
        Serial.println(F("Module: SX1262"));
    #else
        Serial.println(F("Module: Ra-02 (SX1278)"));
    #endif

    Serial.print(F("Frequency: "));
    Serial.print(LORA_FREQUENCY / 1E6);
    Serial.println(F(" MHz"));

    Serial.print(F("Spreading Factor: SF"));
    Serial.println(LORA_SPREADING_FACTOR);

    Serial.print(F("Bandwidth: "));
    Serial.print(LORA_SIGNAL_BANDWIDTH / 1E3);
    Serial.println(F(" kHz"));

    Serial.print(F("Coding Rate: 4/"));
    Serial.println(LORA_CODING_RATE);

    Serial.print(F("TX Power: "));
    Serial.print(LORA_TX_POWER);
    Serial.println(F(" dBm"));

    Serial.print(F("Sync Word: 0x"));
    Serial.println(LORA_SYNC_WORD, HEX);

    Serial.print(F("Pins - NSS: "));
    Serial.print(LORA_NSS);

    #if defined(LORA_MODULE_SX1262)
        Serial.print(F(", DIO1: "));
        Serial.print(LORA_DIO1);
        Serial.print(F(", BUSY: "));
        Serial.print(LORA_BUSY);
    #else
        Serial.print(F(", DIO0: "));
        Serial.print(LORA_DIO0);
    #endif

    Serial.print(F(", RST: "));
    Serial.println(LORA_RESET);

    Serial.println(F("-------------------------"));
}
