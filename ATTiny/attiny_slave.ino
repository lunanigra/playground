#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <TinyWireS.h>

#define DEFAULT_I2C_ADDR 0x20
#define MAX_DATA_LENGTH 0x40

#define LED_RX 3
#define LED_TX 4

volatile uint8_t i2c_regs[] =
{
  DEFAULT_I2C_ADDR, // slave address to store to EEPROM, on next device start this will be the new address
  0xff, // speed 0x00 = fast, 0xff slow
  0x00, // data length
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // data row 1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // data row 2
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // data row 3
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // data row 4
  0xfc, // This last value is a magic number used to check that the data in EEPROM is ours (TODO: switch to some sort of simple checksum ?)
  // ALSO: writing 0x01 to this register triggers saving of data to EEPROM
};

volatile boolean write_eeprom = false;

void regs_eeprom_read()
{
  eeprom_read_block((void*) &i2c_regs, (void*) 0x00, sizeof(i2c_regs));
  if (i2c_regs[sizeof(i2c_regs) - 1] != 0xfc)
  {
    // Insane value, restore default I2C address...
    i2c_regs[0] = DEFAULT_I2C_ADDR;
  }
}

void regs_eeprom_write()
{
  cli();
  eeprom_write_block((const void*) &i2c_regs, (void*) 0x00, sizeof(i2c_regs));
  sei();
}

volatile byte reg_position;

void requestEvent()
{
  TinyWireS.send(i2c_regs[reg_position]);
  // Increment the reg position on each read, and loop back to zero
  reg_position = (reg_position + 1) % sizeof(i2c_regs);
}

void receiveEvent(uint8_t num_bytes)
{
  if (num_bytes < 1)
  {
    // Sanity-check#
    return;
  }

  reg_position = TinyWireS.receive();
  num_bytes--;
  if (!num_bytes)
  {
    // This write was only to set the buffer for next read
    return;
  }
  while(num_bytes--)
  {
    i2c_regs[reg_position] = TinyWireS.receive();
    switch (reg_position)
    {
      case 0x00:
      {
        if (i2c_regs[0] < 0x20 || i2c_regs[0] > 0x2f) i2c_regs[0] = DEFAULT_I2C_ADDR;
        TinyWireS.begin(i2c_regs[0]);
        break;
      }
      case 0x02:
      {
        if (i2c_regs[2] > MAX_DATA_LENGTH) i2c_regs[2] = MAX_DATA_LENGTH;
        break;
      }
    }
    reg_position = (reg_position + 1) % sizeof(i2c_regs);
  }

  // Check for settings store
  if (i2c_regs[sizeof(i2c_regs) - 1] != 0xfc)
  {
    if (i2c_regs[sizeof(i2c_regs) - 1] == 0x01)
    {
      write_eeprom = true;
    }
    // Restore the magic number so it gets written correctly
    i2c_regs[sizeof(i2c_regs) - 1] = 0xfc;
  }
}

void setup()
{
  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);

  regs_eeprom_read();

  if (i2c_regs[0] < 0x20 || i2c_regs[0] > 0x2f) i2c_regs[0] = DEFAULT_I2C_ADDR;
  if (i2c_regs[2] > MAX_DATA_LENGTH) i2c_regs[2] = MAX_DATA_LENGTH;

  TinyWireS.begin(i2c_regs[0]);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
}

uint8_t idx = 0;

void loop()
{
  digitalWrite(LED_TX, i2c_regs[idx + 3] & 0x01);
  idx = (idx + 1) % i2c_regs[2];

  for (uint8_t i = 0; i < i2c_regs[1]; i++)
  {
    delay(1); // Sleep 1ms then check for I2C data

    TinyWireS_stop_check();

    if (write_eeprom)
    {
      // Store the offset etc configuration registers to EEPROM
      write_eeprom = false;
      regs_eeprom_write();
    }
  }
}

