// 4.6 This is a special BMP280 dev ver
#include <lmic.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <hal/hal.h>

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11
#define BMP_CS 10

Adafruit_BMP280 bme;

static const PROGMEM u1_t NWKSKEY[16] = {  };
static const PROGMEM u1_t APPSKEY[16] = {  };
static const PROGMEM u4_t DEVADDR = 0x;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "V4.6";
static osjob_t sendjob;

// Schedule TX every this many seconds 
const unsigned TX_INTERVAL = 300;

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
         case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("RX ack"));
            if (LMIC.dataLen) {
              Serial.println(F("RX "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
         case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
         default:
            Serial.println(F("Othr_Unk event"));
            break;
    }
}

void do_send(osjob_t* j){
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, no TX"));
    } else {
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
       	Serial.print(bme.readTemperature());
        Serial.print(bme.readPressure());
	      Serial.print(bme.readAltitude(1013.5));
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting"));
    Serial.println(F("BMP280 TX Test"));
    if (!bme.begin()) {
	  Serial.println(F("No BMP280"));
  	while (1);
    }
    #ifdef VCC_ENABLE
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    LMIC_reset();

    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif
    
    LMIC_setupChannel(6, 918000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
    LMIC_setupChannel(7, 918200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);
 
    // No link check validation
    LMIC_setLinkCheckMode(0);

    // TTN SF9 RX
    LMIC.dn2Dr = DR_SF9;

    // Set rate TX pwr
        LMIC_setDrTxpow(DR_SF7, 14);

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
