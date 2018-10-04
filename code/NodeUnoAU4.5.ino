/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses ABP (Activation-by-personalisation), where a DevAddr and
 * Session keys are preconfigured (unlike OTAA, where a DevEUI and
 * application key is configured, while the DevAddr and session keys are
 * assigned/generated in the over-the-air-activation procedure).
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!
 *
 * To use this sketch, first register your application and device with
 * the things network, to set or generate a DevAddr, NwkSKey and
 * AppSKey. Each device should have their own unique values for these
 * fields.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 * The body of this code was created by Matthijs Kooijman with
 * adaptions by Andreas Spiess. Thanks guys.
 * I added some of my own stuff for AU 915 freq plan from info on 
 * the TheThingsNetwork web site and other stuff as noted.
 *
 *******************************************************************************/
 // =====================================================
 // REMEMBER TO RESET FRAME COUNTERS WHEN LOAD NEW SKETCH
 // 4.3 Added code to turn on LED as its enter TX cycle
 // and cleaned up REM'ed out stuff. Dig pin 3 & GND.
 // Added attribution to original authors.
 // 4.4 Added SF switch capability between SF7 & SF12 on
 // Dig pin 4 & GND. This is only read on startup.
 // 4.5 Added code to write to serial console when a
 // predetermined code is received inbound.
 // =====================================================

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// Specific To Each User And Device
static const PROGMEM u1_t NWKSKEY[16] = { ADD YOUR KEYS HERE };
static const PROGMEM u1_t APPSKEY[16] = { ADD YOUR KEYS HERE };
static const PROGMEM u4_t DEVADDR = 0xADD YOUR ADDR HERE;

// Define what port the LED will be sitting on
#define MYLED 3
#define SFSWITCH 4

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Sketch V4.5";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 80;

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            digitalWrite(MYLED, LOW);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j){
  
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    digitalWrite(MYLED, HIGH);
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));

    pinMode(MYLED, OUTPUT);
    pinMode(SFSWITCH, INPUT_PULLUP);
    digitalWrite(MYLED, HIGH);

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters.
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.

    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Set up the channels used in Australia
    
    LMIC_setupChannel(0, 916800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(1, 917000000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      //
    LMIC_setupChannel(2, 917200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(3, 917400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(4, 917600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(5, 917800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(6, 918000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    LMIC_setupChannel(7, 918200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      //
    // LMIC_setupChannel(8, 917500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_MILLI);      // 
 
    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    // LMIC_setDrTxpow(DR_SF12,14);
    if (digitalRead(SFSWITCH) == HIGH) {
        Serial.println("SF7");
        LMIC_setDrTxpow(DR_SF7, 14);
    }
    else  {
        Serial.println("SF12");
        LMIC_setDrTxpow(DR_SF12, 14);
    }

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
