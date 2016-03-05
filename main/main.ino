//NFC Shield Libraries
#include <PN532Interface.h>
#include <PN532.h>
#include <PN532_I2C.h>
//Clock lib
#include <DS1302.h>

//OLED Libraries
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

//NFC Shield Global Vars
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

//OLED Global Var
#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

namespace {

// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. See the DS1302
// datasheet:
//
//   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
const int RST   = 10;  // Chip Enable
const int SDA= 9;  // Input/Output
const int SCLK = 8;  // Serial Clock

// Create a DS1302 object.
DS1302 rtc(RST, SDA, SCLK);
 Time t = rtc.time();
}

void setup() {

  Serial.begin(115200);
  rtc.writeProtect(false);
  rtc.halt(false);
  //Init OLED
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(TimesNewRoman16);
  oled.clear();
  oled.println("Initializing");


  //Init NFC Shield
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);

  // configure board to read RFID tags
  nfc.SAMConfig();

}

void loop()
{
  bool success;
 // printTime();
  uint8_t responseLength = 48;

  Serial.println("Waiting for an ISO14443A card");
  oled.clear();
  oled.println("Swipe Pass");

  // set shield to inListPassiveTarget
  success = nfc.inListPassiveTarget();

  if(success) {

     Serial.println("Found something!");

    uint8_t selectApdu[] = { 0x00, /* CLA */
                              0xA4, /* INS */
                              0x04, /* P1  */
                              0x00, /* P2  */
                              0x07, /* Length of AID  */
                              0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x16, /* AID defined on Android App */
                              0x00  /* Le  */ };

    uint8_t response[responseLength];
    String payload = "";
    String message = "";
    int headerSize, passType;

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    if(success) {

      oled.clear();
     
      /*
       * Reply Format = XYZZZZZZZZZ...ZZZZZ
       * X = Pass Status. 0x30 = 0, Invalid Pass. 0x31 = 1, Valid Pass
       * Y = Pass Type. 0x30 = 0, Monthly pass. 0x31 = 1, Per Rides pass
       * Z = Payload
       * 
       * If X is 0, then Y does not exist. The payload starts at byte 1 rather than 2. Byte
       * 0 is always the pass status
       */
      if(response[0] == 0x30) {
        headerSize = 1;
        passType = -1;
      } else if (response[0] == 0x31) {
        headerSize = 2;
        passType = response[1];
      } else {
        headerSize = -1;
      }

      if (headerSize != -1) {
        for(int i=headerSize; i<responseLength; i++) {
          payload += char(response[i]);
        }
       
        if(passType == -1) {
          oled.println(payload);
        } else if (passType == 0x30) {
          oled.println("Expires On: ");
          oled.println(payload);
        } else if(passType == 0x31) {
          oled.println(payload);
          oled.println("rides remaining");
        } else {
          oled.println("Please try again");
        }

        Serial.print("Message: "); Serial.println(message); 
        delay(3000);
      }
    }
    else {
      Serial.println("Failed sending SELECT AID");
    }
  }
  else {
    Serial.println("Didn't find anything!");
  }

  delay(1000);
}

void setupNFC() {

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

