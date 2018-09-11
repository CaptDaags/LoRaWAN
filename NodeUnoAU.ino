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

#include <lmic.h>
#include <hal/hal.h>
//#include <credentials.h>

//#ifdef CREDENTIALS
//static const u1_t NWKSKEY[16] = NWKSKEY1;
//static const u1_t APPSKEY[16] = APPSKEY1;
//static const u4_t DEVADDR = DEVADDR1;
//#else
// static const u1_t NWKSKEY[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// static const u1_t APPSKEY[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// static const u4_t DEVADDR = 0x00000000;
static const u1_t NWKSKEY[16] = {0xCB,0x06,0x21,0xD9,0xD8,0x93,0x93,0x9D,0x45,0x2A,0x00,0x91,0xCC,0x3E,0xAD,0x57};
static const u1_t APPSKEY[16] = {0x4A,0x2B,0x91,0x42,0xA9,0xD9,0xEB,0x7B,0x53,0x6F,0xE0,0x0D,0x5B,0xE1,0x1F,0x3A};
static const u4_t DEVADDR = 0x260418D5;
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
const unsigned TX_INTERVAL = 20;

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
    LMIC_setSession (0x1, DEVADDR, (uint8_t*)NWKSKEY, (uint8_t*)APPSKEY);

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
