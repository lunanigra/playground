/*************************************************** 
  This is an example for the Adafruit CC3000 Wifi Breakout & Shield

  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 /*
This example does a test of the TCP client capability:
  * Initialization
  * Optional: SSID scan
  * AP connection
  * DHCP printout
  * DNS lookup
  * Optional: Ping
  * Connect to website and print out webpage contents
  * Disconnect
SmartConfig is still beta and kind of works but is not fully vetted!
It might not work on all networks!
*/
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include <Adafruit_NeoPixel.h>
#include <avr/wdt.h>

#define DEVICE_ID "demo"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  A3
#define ADAFRUIT_CC3000_CS    8
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

// #define WLAN_SSID       "Hack"           // cannot be longer than 32 characters!
// #define WLAN_PASS       "Hackme123"
#define WLAN_SSID       "SPARK"           // cannot be longer than 32 characters!
#define WLAN_PASS       "IsaacNewt0n"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  1000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

#define PIN 3

// What page to grab!
#define WEBSITE      "medicall.craigmorris.io"


/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

uint32_t ip;
int buttonState;
long lastKeepAlive = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

void setup(void) {
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));

  if (!cc3000.begin()) {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    colorWipe(strip.Color(255, 0, 0), 0); // Red
    while(1);
  }

  // Optional SSID scan
  // listSSIDResults();

  connectWiFi();

  retrieveIpAddress();
  cc3000.printIPdotsRev(ip);
  
  colorWipe(strip.Color(0, 255, 0), 0); // Green
}

void loop(void) {

  int reading = analogRead(A1);
  // Serial.print("\n\nA1: ");
  // Serial.println(reading);
  
  int tmpButtonState = 0;
  
  if (reading > 650) { // Read switch 1
     tmpButtonState = 3;
  }
  if (reading > 850) { // Read switch 2
     tmpButtonState = 2;
  }
  if (reading > 930) { // Read switch 3
     tmpButtonState = 1;
  }

  if (tmpButtonState != buttonState) {
    buttonState = tmpButtonState;
    if (buttonState != 0) {
      Serial.print("\n\nSwitch: ");
      Serial.println(buttonState);
      handleRequest(buttonState);
      lastKeepAlive = millis();
    }
  }

  if (millis() - lastKeepAlive > 60 * 1000l) {
    keepAlive();
    lastKeepAlive = millis();
  }

  delay(50);
}

void connectWiFi(void) {
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  colorWipe(strip.Color(0, 0, 255), 0); // Blue
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    colorWipe(strip.Color(255, 0, 0), 0); // Red
    while(1);
  }
   
  Serial.println(F("Connected!"));
  colorWipe(strip.Color(255, 255, 0), 0); // Yellow

  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    colorWipe(strip.Color(255, 0, 0), 0); // Red
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */  
  while (!displayConnectionDetails()) {
    colorWipe(strip.Color(255, 0, 0), 0); // Red
    delay(1000);
  }
}

void retrieveIpAddress(void) {
  ip = 0;
  int cnt = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
    if (cnt++ > 10) reboot();
  }
}

void handleRequest(int box) {
  Serial.println(F("\n\nHandle request"));
  colorWipe(strip.Color(255, 0, 0), 0); // Red
  
  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint("/v1/open/");
    www.fastrprint(DEVICE_ID);
    www.fastrprint("/");
    www.write(0x30 + box);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));
    connectWiFi(); // Try to reconnect...
    colorWipe(strip.Color(0, 255, 0), 0); // Green
    return;
  }

  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("-------------------------------------"));

  colorWipe(strip.Color(0, 255, 0), 0); // Green
}

void keepAlive(void) {
  colorWipe(strip.Color(255, 255, 0), 0); // Orange

  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint("/v1/keepalive/");
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));
    connectWiFi(); // Try to reconnect...
    colorWipe(strip.Color(0, 255, 0), 0); // Green
    return;
  }
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      lastRead = millis();
    }
  }
  www.close();

  colorWipe(strip.Color(0, 255, 0), 0); // Green
}


/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void) {
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void) {
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  } else {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void reboot(void) {
  asm volatile ("  jmp 0");
}

