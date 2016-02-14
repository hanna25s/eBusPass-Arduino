//NFC Shield Libraries
#include <PN532Interface.h>
#include <PN532.h>
#include <PN532_I2C.h>

//OLED Libraries
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

//NFC Shield Global Vars
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

//OLED Global Var
#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

void setup() {

  Serial.begin(115200);

  //Init OLED
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();


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

  uint8_t responseLength = 48;

  Serial.println("Waiting for an ISO14443A card");

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
    uint8_t payload[responseLength];
    int headerSize;

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    if(success) {
      
      //0x30 = 0 in ASCII, which indicates an invalid pass.
      if(response[0] == 0x30) {
        headerSize = 1;
      } else {
        headerSize = 2;
      }

      for(int i=headerSize; i<responseLength; i++) {
        payload[i-headerSize] = response[i];
      }
      
      nfc.PrintHexChar(payload, responseLength - headerSize);
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

void printResponse(uint8_t *response, uint8_t responseLength) {

   String respBuffer;

    for (int i = 0; i < responseLength; i++) {

      if (response[i] < 0x10)
        respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10

      respBuffer = respBuffer + String(response[i], HEX) + " ";
    }

    Serial.print("response: "); Serial.println(respBuffer);
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


