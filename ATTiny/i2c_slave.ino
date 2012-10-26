#include <avr/wdt.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <TinyWireS.h>

#define DEFAULT_I2C_ADDR 0x20
#define MAX_SIZE 0x80

#define LED_RX 3
#define LED_TX 4

#define CMD_ADDR 0x41
#define CMD_GET_DATA 0x47
#define CMD_SET_DATA 0x53

byte cmd = 1;

struct config_t
{
  uint8_t address;
  uint8_t count;
  uint8_t leds[MAX_SIZE];
} configuration;

void setup()
{
  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);

  EEPROM_readAnything(0x00, configuration);

  if (configuration.address < 0x20 || configuration.address > 0x2f) configuration.address = DEFAULT_I2C_ADDR;
  if (configuration.count > MAX_SIZE) configuration.count = MAX_SIZE;

  digitalWrite(LED_TX, HIGH);
  delay(250);
  digitalWrite(LED_TX, LOW);
  delay(250);

  TinyWireS.begin(configuration.address);
}

void loop()
{
  if (TinyWireS.available())
  {
    handleCommuncationRequest();
  }

  delay(100); // e.g. do something else
}

void handleCommuncationRequest()
{
  uint8_t numBytes = TinyWireS.available();
  if (numBytes < 1)
    return;
  
  cmd = TinyWireS.receive();
  digitalWrite(LED_RX, HIGH);
  delay(5);
  digitalWrite(LED_RX, LOW);
  
  if (cmd == CMD_ADDR && numBytes == 2)
  {
    uint8_t address = TinyWireS.receive();
    // TODO set new address in EEPROM
    softReset(); // do restart
  }
  else if (cmd = CMD_GET_DATA)
  {
    uint8_t crc = 0xff;
    for (uint8_t i = 0; i < MAX_SIZE; i++)
    {
      crc = crc_ibutton_update(crc, configuration.leds[i]);
      TinyWireS.send(configuration.leds[i]); 
    }
    TinyWireS.send(crc);
  }
  else
  {
    // TODO handle further commands
    while (TinyWireS.available())
    {
      TinyWireS.receive();
    }
  }
}

uint8_t crc_ibutton_update(uint8_t crc, uint8_t data)
{
  crc = crc ^ data;
  for (uint8_t i = 0; i < 8; i++)
  {
    if (crc & 0x01)
      crc = (crc >> 1) ^ 0x8C;
    else
      crc >>= 1;
  }

  return crc;
}

void softReset()
{
  // reset via watchdog timer
  cli();
  wdt_enable(WDTO_15MS);
  while (1);
}
