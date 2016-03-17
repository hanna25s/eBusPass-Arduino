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

#define SECURE_KEY "6Qo25p1DJkX"
#define KEY_LENGTH sizeof(SECURE_KEY)
#define YEAR_END KEY_LENGTH + 3
#define MONTH_END YEAR_END + 3
#define DAY_END MONTH_END + 3

namespace {
const int RST   = 10;  // Chip Enable
const int SDA= 9;  // Input/Output
const int SCLK = 8;  // Serial Clock

// Create a DS1302 object.
DS1302 rtc(RST, SDA, SCLK);
 Time t = rtc.time();
}

void setup() 
{

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
  uint8_t responseLength = 128;

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

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    char message[responseLength];
    int decryptedMessageLength = responseLength/4;
    char decrypted[decryptedMessageLength];
    
    if(success) {
      for(int i=0; i<=responseLength; i+=2) {
        message[i/2] = char(hexToInt(response[i]) << 4 | hexToInt(response[i+1]));
      }

      decrypt(decrypted, message, sizeof(decrypted));

      for(int i=0; i<sizeof(decrypted); i++) {
        Serial.print(decrypted[i]);
      }
      Serial.println();

      oled.clear();
      String key = "";
      String mDay = "";
      String mMonth = "";
      String mYear = "";
      String rides = "";
      uint8_t i = 0;
      
      for(; i<KEY_LENGTH-1; i++) {
        key += char(decrypted[i]);
      }
      
      if(key == SECURE_KEY) {
        for(; i<YEAR_END; i++) {
          mYear += char(decrypted[i]);
        }
        i++;
        for(; i<MONTH_END; i++) {
          mMonth += char(decrypted[i]);
        }
        i++;
        for(; i<DAY_END; i++) {
          mDay += char(decrypted[i]);
        }
        for(; i<decryptedMessageLength; i++) {
          rides += char(decrypted[i]);
        }

        boolean isMonthlyValid = false;

        if(mYear.toInt() > t.yr) {
          isMonthlyValid = true;
        } else if(mYear.toInt() == t.yr && 
                  mMonth.toInt() >= t.mon &&
                  mDay.toInt() >= t.day) {
          isMonthlyValid = true;            
        }
     
        if(isMonthlyValid) {
          oled.println("Expires on: ");
          oled.println(mYear + "/" + mMonth + "/" + mDay);
          delay(3000);
        } else {
          if(rides.toInt() > 0) {
            oled.print(rides.toInt() - 1); oled.println(" rides remaining");
            
            uint8_t apdu[] = "ReduceRides";
            uint8_t back[32];
            uint8_t length = 32; 
            success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);

            delay(3000);
          } else {
            oled.println("No Pass"); 
          }
        }
      } else {
        oled.println("Insecure access");
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

int hexToInt(char x) {

  if(x <= '9')
      return x - '0';
  if(x <= 'F')
      return x - 'A' + 10;
  if(x <= 'f')
      return x - 'a' + 10;

  return 0;
  
}


/*
 * Originally written by: nikkotorcita
 * https://github.com/nikkotorcita/RSA_arduino_library
 * 
 * Adapted slightly to work with our input from the Android phone
 */
void decrypt(char *plainText, char *cipherText, int decryptedLength)
{
   long M = 1;
   int n = 14351;
   int d = 1283;
   int temp = 0;
   int ctr = 0;

   for(int i = 0; i < decryptedLength; i++) {
       ctr = i * sizeof(int);
       temp = (((unsigned char)cipherText[ctr + 1] << 8) | (unsigned char)cipherText[ctr]);
       
       for(int j = 0; j < d; j++) {
           M = (M * temp) % n;
       }

       plainText[i] = (unsigned char)(M & 0xFF); 
       M = 1;
   }
} 
