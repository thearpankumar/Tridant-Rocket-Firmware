#include "DualLoRaComm.h"

DualLoRaComm::DualLoRaComm() : initialized(false) {
    // Initialize device names from build flags
    deviceNames[MODULE_1] = LORA1_NAME;
    deviceNames[MODULE_2] = LORA2_NAME;

    // Initialize pin configurations from build flags
    nssPins[MODULE_1] = LORA1_NSS;
    nssPins[MODULE_2] = LORA2_NSS;

    resetPins[MODULE_1] = LORA1_RESET;
    resetPins[MODULE_2] = LORA2_RESET;

    #if defined(LORA_MODULE_SX1262)
        dio1Pins[MODULE_1] = LORA1_DIO1;
        dio1Pins[MODULE_2] = LORA2_DIO1;
        busyPins[MODULE_1] = LORA1_BUSY;
        busyPins[MODULE_2] = LORA2_BUSY;
    #else
        dio0Pins[MODULE_1] = LORA1_DIO0;
        dio0Pins[MODULE_2] = LORA2_DIO0;
    #endif

    // Initialize pointers to null
    for (int i = 0; i < NUM_LORA_MODULES; i++) {
        radioModules[i] = nullptr;
        radios[i] = nullptr;
    }
}

DualLoRaComm::~DualLoRaComm() {
    for (int i = 0; i < NUM_LORA_MODULES; i++) {
        if (radios[i] != nullptr) {
            delete radios[i];
            radios[i] = nullptr;
        }
        if (radioModules[i] != nullptr) {
            delete radioModules[i];
            radioModules[i] = nullptr;
        }
    }
}

bool DualLoRaComm::begin() {
    Serial.println(F("\n=== Dual LoRa Module Initialization ==="));

    // Print module type
    #if defined(LORA_MODULE_SX1262)
        Serial.println(F("Module Type: SX1262"));
    #else
        Serial.println(F("Module Type: Ra-02 (SX1278)"));
    #endif

    // Initialize SPI bus (shared between both modules)
    Serial.println(F("Initializing shared SPI bus..."));
    SPI.begin();
    delay(50);

    // Deselect both modules first
    pinMode(nssPins[MODULE_1], OUTPUT);
    pinMode(nssPins[MODULE_2], OUTPUT);
    digitalWrite(nssPins[MODULE_1], HIGH);
    digitalWrite(nssPins[MODULE_2], HIGH);

    // Initialize Module 1
    Serial.println(F("\n--- Initializing Module 1 ---"));
    if (!initModule(MODULE_1)) {
        Serial.println(F("ERROR: Failed to initialize Module 1"));
        return false;
    }
    Serial.print(F("Module 1 ("));
    Serial.print(deviceNames[MODULE_1]);
    Serial.println(F(") initialized successfully"));

    // Small delay between module initializations
    delay(100);

    // Initialize Module 2
    Serial.println(F("\n--- Initializing Module 2 ---"));
    if (!initModule(MODULE_2)) {
        Serial.println(F("ERROR: Failed to initialize Module 2"));
        return false;
    }
    Serial.print(F("Module 2 ("));
    Serial.print(deviceNames[MODULE_2]);
    Serial.println(F(") initialized successfully"));

    initialized = true;
    Serial.println(F("\n=== Both modules initialized successfully ==="));
    return true;
}

bool DualLoRaComm::initModule(uint8_t index) {
    if (index >= NUM_LORA_MODULES) {
        return false;
    }

    Serial.print(F("  Name: "));
    Serial.println(deviceNames[index]);
    Serial.print(F("  NSS: GPIO"));
    Serial.println(nssPins[index]);
    Serial.print(F("  RESET: GPIO"));
    Serial.println(resetPins[index]);

    #if defined(LORA_MODULE_SX1262)
        Serial.print(F("  DIO1: GPIO"));
        Serial.println(dio1Pins[index]);
        Serial.print(F("  BUSY: GPIO"));
        Serial.println(busyPins[index]);

        // Create RadioLib Module instance for SX1262
        radioModules[index] = new Module(nssPins[index], dio1Pins[index], resetPins[index], busyPins[index]);
    #else
        Serial.print(F("  DIO0: GPIO"));
        Serial.println(dio0Pins[index]);

        // Create RadioLib Module instance for SX1278
        radioModules[index] = new Module(nssPins[index], dio0Pins[index], resetPins[index], RADIOLIB_NC);
    #endif

    // Create radio instance
    radios[index] = new LoRaModuleType(radioModules[index]);

    // Initialize module (RadioLib begin() takes frequency in MHz)
    int state = radios[index]->begin(LORA_FREQUENCY / 1E6);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Radio initialization failed with code: "));
        Serial.println(state);
        Serial.println(F("  Check wiring and connections"));
        return false;
    }

    #if defined(LORA_MODULE_SX1262)
        // SX1262-specific configuration

        // Set TCXO voltage with 5ms timeout
        state = radios[index]->setTCXO(1.6, 5000);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("  TCXO configured at 1.6V"));
        } else {
            Serial.print(F("  NOTE: TCXO not available (code: "));
            Serial.print(state);
            Serial.println(F(") - using crystal oscillator"));
        }

        // Use DC-DC regulator (more efficient)
        state = radios[index]->setRegulatorDCDC();
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("  WARNING: Failed to set DC-DC regulator, code: "));
            Serial.println(state);
        }

        // Set DIO2 as RF switch control
        state = radios[index]->setDio2AsRfSwitch(true);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("  WARNING: Failed to set DIO2 as RF switch, code: "));
            Serial.println(state);
        }

        // Set current limit (max 140mA for SX1262)
        state = radios[index]->setCurrentLimit(140.0);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("  WARNING: Failed to set current limit, code: "));
            Serial.println(state);
        }
    #endif

    // Configure LoRa parameters
    if (!configureModule(radios[index])) {
        return false;
    }

    return true;
}

bool DualLoRaComm::configureModule(LoRaModuleType* radio) {
    int state;

    state = radio->setSpreadingFactor(LORA_SPREADING_FACTOR);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to set spreading factor, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setBandwidth(LORA_SIGNAL_BANDWIDTH / 1E3);  // RadioLib uses kHz
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to set bandwidth, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setCodingRate(LORA_CODING_RATE);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to set coding rate, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setPreambleLength(LORA_PREAMBLE_LENGTH);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to set preamble length, code: "));
        Serial.println(state);
        return false;
    }

    state = radio->setSyncWord(LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to set sync word, code: "));
        Serial.println(state);
        return false;
    }

    #if defined(LORA_MODULE_SX1262)
        // SX1262: Use lower power if high power PA fails
        state = radio->setOutputPower(LORA_TX_POWER);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("  WARNING: TX power "));
            Serial.print(LORA_TX_POWER);
            Serial.println(F(" dBm failed, trying 14 dBm"));
            state = radio->setOutputPower(14);
            if (state != RADIOLIB_ERR_NONE) {
                Serial.print(F("  ERROR: Failed to set TX power, code: "));
                Serial.println(state);
                return false;
            }
        }
    #else
        state = radio->setOutputPower(LORA_TX_POWER);
        if (state != RADIOLIB_ERR_NONE) {
            Serial.print(F("  ERROR: Failed to set TX power, code: "));
            Serial.println(state);
            return false;
        }
    #endif

    state = radio->setCRC(true);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("  ERROR: Failed to enable CRC, code: "));
        Serial.println(state);
        return false;
    }

    return true;
}

void DualLoRaComm::selectModule(uint8_t moduleIndex) {
    // Deselect all modules first
    digitalWrite(nssPins[MODULE_1], HIGH);
    digitalWrite(nssPins[MODULE_2], HIGH);

    // Select the requested module
    if (moduleIndex < NUM_LORA_MODULES) {
        digitalWrite(nssPins[moduleIndex], LOW);
    }
}

bool DualLoRaComm::sendPacket(uint8_t moduleIndex, const uint8_t* data, size_t length) {
    if (!initialized) {
        Serial.println(F("ERROR: DualLoRaComm not initialized"));
        return false;
    }

    if (moduleIndex >= NUM_LORA_MODULES) {
        Serial.println(F("ERROR: Invalid module index"));
        return false;
    }

    if (radios[moduleIndex] == nullptr) {
        Serial.println(F("ERROR: Radio not initialized"));
        return false;
    }

    if (length == 0 || length > 255) {
        Serial.println(F("ERROR: Invalid packet length"));
        return false;
    }

    // Transmit the packet (blocking)
    int state = radios[moduleIndex]->transmit((uint8_t*)data, length);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("ERROR: Packet transmission failed on module "));
        Serial.print(moduleIndex);
        Serial.print(F(", code: "));
        Serial.println(state);
        return false;
    }

    return true;
}

const char* DualLoRaComm::getDeviceName(uint8_t moduleIndex) {
    if (moduleIndex >= NUM_LORA_MODULES) {
        return "Unknown";
    }
    return deviceNames[moduleIndex];
}

void DualLoRaComm::printConfig() {
    Serial.println(F("\n=== Dual LoRa Configuration ==="));

    #if defined(LORA_MODULE_SX1262)
        Serial.println(F("Module Type: SX1262"));
    #else
        Serial.println(F("Module Type: Ra-02 (SX1278)"));
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

    Serial.println(F("\n--- Module 1 ---"));
    Serial.print(F("Name: "));
    Serial.println(deviceNames[MODULE_1]);
    Serial.print(F("NSS: GPIO"));
    Serial.print(nssPins[MODULE_1]);
    #if defined(LORA_MODULE_SX1262)
        Serial.print(F(", DIO1: GPIO"));
        Serial.print(dio1Pins[MODULE_1]);
        Serial.print(F(", BUSY: GPIO"));
        Serial.print(busyPins[MODULE_1]);
    #else
        Serial.print(F(", DIO0: GPIO"));
        Serial.print(dio0Pins[MODULE_1]);
    #endif
    Serial.print(F(", RST: GPIO"));
    Serial.println(resetPins[MODULE_1]);

    Serial.println(F("\n--- Module 2 ---"));
    Serial.print(F("Name: "));
    Serial.println(deviceNames[MODULE_2]);
    Serial.print(F("NSS: GPIO"));
    Serial.print(nssPins[MODULE_2]);
    #if defined(LORA_MODULE_SX1262)
        Serial.print(F(", DIO1: GPIO"));
        Serial.print(dio1Pins[MODULE_2]);
        Serial.print(F(", BUSY: GPIO"));
        Serial.print(busyPins[MODULE_2]);
    #else
        Serial.print(F(", DIO0: GPIO"));
        Serial.print(dio0Pins[MODULE_2]);
    #endif
    Serial.print(F(", RST: GPIO"));
    Serial.println(resetPins[MODULE_2]);

    Serial.println(F("================================"));
}
