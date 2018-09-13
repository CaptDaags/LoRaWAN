// MIT License
// https://github.com/gonzalocasas/arduino-uno-dragino-lorawan/blob/master/LICENSE
// Based on examples from https://github.com/matthijskooijman/arduino-lmic
// Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

// Adaptions: Andreas Spiess
//          : Darrin Pearce - Was throwing compile errors until I changed
//                               LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
//                            to LMIC_setSession (0x1, DEVADDR, (uint8_t*)NWKSKEY, (uint8_t*)APPSKEY);
//                            which seemed to make sense even though I had no idea what I was doing
//                            this has yet to be actually uploaded and tested so may be bollocks.
//                            Source of this wisdom: https://github.com/things4u/LoRa-LMIC-1.51/issues/13
//                            As you can see, like keeping orginal stuff in REM's in case I fuck it up.
//            Darrin Pearce - Now operational, had to put back the PROGMEM stuff in the setup and also list out
//                            the AU freq's to make sure we are TX'ing on the same as the gateway is listening.
//                            The freq data was from the global in /opt/ttngateway/bin on the gateway Pi.

#include <lmic.h>
#include <hal/hal.h>
//#include <credentials.h>

//#ifdef CREDENTIALS
//static const u1_t NWKSKEY[16] = NWKSKEY1;
//static const u1_t APPSKEY[16] = APPSKEY1;
//static const u4_t DEVADDR = DEVADDR1;
//#else
// Examples of the format of the key values
// static const u1_t NWKSKEY[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// static const u1_t APPSKEY[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// static const u4_t DEVADDR = 0x00000000;
static const u1_t NWKSKEY[16] = ADD YOUR KEYS;
static const u1_t APPSKEY[16] = ADD YOUR KEYS;
static const u4_t DEVADDR = 0xADD YOUR KEYS;
//#endif

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 300;

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) {
        Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
        // Schedule next transmission
        os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}

void do_send(osjob_t* j){
    // Payload to send (uplink)
    Serial.println(F("Entering SEND..."));
    static uint8_t message[] = "hi";

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, message, sizeof(message)-1, 0);
        Serial.println(F("Sending uplink packet..."));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting..."));

    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters.
    // LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    // LMIC_setSession (0x1, DEVADDR, (uint8_t*)NWKSKEY, (uint8_t*)APPSKEY);

     // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

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
    LMIC_setDrTxpow(DR_SF12,14);

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
