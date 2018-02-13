/*
  SmartShow AirPixel ONE - Single Universe ArtNet to WS2812 Driver - For Wemos D1
  You can set the Device IP, and universe number below
  Works perfectly with Jinx LED software
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

/* Begin user config stuff */

// Pixel setup
#define WSout 5  // Pin for yo pix (default is 5 = D1)

// Status indication ONLY
#define INDICATOR_SIZE 10 // How many pixels to blink for status indicator
#define INDICATOR_BRIGHT 20 // How bright you want your status blinks? (0-255)

// WiFi Setup
const char* ssid     = "SSID";
const char* password = "Password";
const char* dnsname  = "Test8266"; // used for OTA

IPAddress local_ip(192, 168, 1, 123);
IPAddress gateway_ip(192, 168, 1, 1);
IPAddress subnet_ip(255, 255, 255, 0);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(INDICATOR_SIZE, WSout, NEO_GRB + NEO_KHZ800);

/* End user config stuff */
#define WSbit (1<<WSout)

// ARTNET CODES
#define ARTNET_DATA 0x50
#define ARTNET_POLL 0x20
#define ARTNET_POLL_REPLY 0x21
#define ARTNET_PORT 6454
#define ARTNET_HEADER 17

WiFiUDP udp;

uint8_t uniData[514];
uint16_t uniSize;
uint8_t hData[ARTNET_HEADER + 1];
uint8_t net = 0;
uint8_t universe = 0;
uint8_t subnet = 0;
uint16_t lastArt = 0; // Counter we'll use for time between ArtNet messages

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.config(local_ip, gateway_ip, subnet_ip);

  //Pix setup
  strip.begin();
  strip.setBrightness(INDICATOR_BRIGHT);
  strip.show(); // Initialize all pixels to 'off'
  int pos = INDICATOR_SIZE + 3, dir = -1; // Position, direction of "eye"

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    int j;

    // Draw 5 pixels centered on pos.  setPixelColor() will clip any
    // pixels off the ends of the strip, we don't need to watch for that.
    strip.setPixelColor(pos - 3, 0x000000); // Off
    strip.setPixelColor(pos - 2, 0x100000); // Dark red
    strip.setPixelColor(pos - 1, 0x800000); // Medium red
    strip.setPixelColor(pos    , 0xFF3000); // Center pixel is brightest
    strip.setPixelColor(pos + 1, 0x800000); // Medium red
    strip.setPixelColor(pos + 2, 0x100000); // Dark red
    strip.setPixelColor(pos + 3, 0x000000); // Off
    strip.show();
    delay(60);

    // Rather than being sneaky and erasing just the tail pixel,
    // it's easier to erase it all and draw a new one next time.
    for (j = -2; j <= 2; j++) strip.setPixelColor(pos + j, 0);

    // Bounce off ends of strip
    pos += dir;
    if (pos < -3) {
      pos = INDICATOR_SIZE + 3;
      delay(2000);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");

  /* OTA Stuff Begins... */
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(dnsname);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  /* OTA Stuff Ends. */

  udp.begin(ARTNET_PORT); // Open ArtNet port

  pinMode(WSout, OUTPUT);
}

void ICACHE_FLASH_ATTR sendWS() {
  uint32_t writeBits;
  uint8_t  bitMask, time;
  os_intr_lock();
  for (uint16_t t = 0; t < uniSize; t++) { // outer loop counting bytes
    bitMask = 0x80;
    while (bitMask) {
      // time=0ns : start by setting bit on
      time = 4;
      while (time--) {
        WRITE_PERI_REG( 0x60000304, WSbit );  // do ON bits // T=0
      }
      if ( uniData[t] & bitMask ) {
        writeBits = 0;  // if this is a '1' keep the on time on for longer, so dont write an off bit
      }
      else {
        writeBits = WSbit;  // else it must be a zero, so write the off bit !
      }
      time = 4;
      while (time--) {
        WRITE_PERI_REG( 0x60000308, writeBits );  // do OFF bits // T='0' time 350ns
      }
      time = 6;
      while (time--) {
        WRITE_PERI_REG( 0x60000308, WSbit );  // switch all bits off  T='1' time 700ns
      }
      // end of bite write time=1250ns
      bitMask >>= 1;
    }
  }
  os_intr_unlock();
}

void loop() {
  ArduinoOTA.handle();
  if (udp.parsePacket()) {
    udp.read(hData, ARTNET_HEADER + 1);
    if ( hData[0] == 'A' && hData[1] == 'r' && hData[2] == 't' && hData[3] == '-' && hData[4] == 'N' && hData[5] == 'e' && hData[6] == 't') {
      if ( hData[8] == 0x00 && hData[9] == ARTNET_DATA && hData[15] == net ) {
        lastArt = millis();
        if ( hData[14] == (subnet << 4) + universe ) { // UNIVERSE
          if (!uniSize) {
            uniSize = (hData[16] << 8) + (hData[17]);
          }
          udp.read(uniData, uniSize);
          Serial.print("ArtNet packet RX Uni 0 - size:");
          Serial.println(uniSize);
          sendWS();
        }
      } // if Artnet Data
    }
  }
  if ((millis() - lastArt) > 5000) {
    //Serial.println("Waiting on ArtNet command... tum ti tum");
    WiFiBlink();
  }
}

/* Sample the WiFi strength and make a blinking indicator to show strength
   Based on this guide: https://eyesaas.com/wi-fi-signal-strength/
     red = bad (under -70dB)
     yellow = ok (over -67dB)
     green = good (over -60dB)
     blue = WTF
*/
void WiFiBlink()
{
  static uint32_t tick = 0;
  if ( millis() - tick < 2000) {
    return;
  }

  int32_t rssi = WiFi.RSSI();
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");

  // Take control of the pin
  strip.setPin(5);
  uint32_t WifiStrength;
  if (rssi > -60) {
    WifiStrength = strip.Color(0, 255, 0);
  }
  else if (rssi > -67) {
    WifiStrength = strip.Color(255, 255, 0);
  }
  else if (rssi > -70) {
    WifiStrength = strip.Color(255, 50, 0);
  }
  else  WifiStrength = strip.Color(0, 0, 100);
  // Blink our WiFi signal strength indicator
  for (uint16_t i = 0; i < INDICATOR_SIZE; i++) {
    strip.setPixelColor(i, WifiStrength);
  }
  strip.show();
  delay(100);
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
  tick = millis();
  pinMode(WSout, OUTPUT);
}

