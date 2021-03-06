#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <TinyWireS.h>

#define DEFAULT_I2C_ADDR 0x20

#define LED_1 0
#define LED_2 1
#define LED_3 2
#define LED_4 3
#define LED_5 5
#define LED_6 7
#define LED_7 10
#define LED_8 9

volatile uint8_t i2c_regs[] = {
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

void regs_eeprom_read() {
  eeprom_read_block((void*) &i2c_regs, (void*) 0x00, sizeof(i2c_regs));
  if (i2c_regs[sizeof(i2c_regs) - 1] != 0xfc) {
    // Insane value, restore default I2C address...
    i2c_regs[0] = DEFAULT_I2C_ADDR;
  }
}

void regs_eeprom_write() {
  cli();
  eeprom_write_block((const void*) &i2c_regs, (void*) 0x00, sizeof(i2c_regs));
  sei();
}

volatile byte reg_position;

void requestEvent() {
  TinyWireS.send(i2c_regs[reg_position]);
  // Increment the reg position on each read, and loop back to zero
  reg_position = (reg_position + 1) % sizeof(i2c_regs);
}

void receiveEvent(uint8_t num_bytes) {
  if (num_bytes < 1) {
    // Sanity-check#
    return;
  }

  reg_position = TinyWireS.receive();
  num_bytes--;
  if (!num_bytes) {
    // This write was only to set the buffer for next read
    return;
  }
  while(num_bytes--) {
    i2c_regs[reg_position] = TinyWireS.receive();
    switch (reg_position) {
      case 0x00: {
        if (i2c_regs[0] < 0x20 || i2c_regs[0] > 0x2f) i2c_regs[0] = DEFAULT_I2C_ADDR;
        TinyWireS.begin(i2c_regs[0]);
        break;
      }
      case 0x02: {
        if (i2c_regs[2] > sizeof(i2c_regs)) i2c_regs[2] = sizeof(i2c_regs);
        break;
      }
    }
    reg_position = (reg_position + 1) % sizeof(i2c_regs);
  }

  // Check for settings store
  if (i2c_regs[sizeof(i2c_regs) - 1] != 0xfc) {
    if (i2c_regs[sizeof(i2c_regs) - 1] == 0x01) {
      write_eeprom = true;
    }
    // Restore the magic number so it gets written correctly
    i2c_regs[sizeof(i2c_regs) - 1] = 0xfc;
  }
}

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_5, OUTPUT);
  pinMode(LED_6, OUTPUT);
  pinMode(LED_7, OUTPUT);
  pinMode(LED_8, OUTPUT);

  regs_eeprom_read();

  if (i2c_regs[0] < 0x20 || i2c_regs[0] > 0x2f) i2c_regs[0] = DEFAULT_I2C_ADDR;
  if (i2c_regs[2] > sizeof(i2c_regs)) i2c_regs[2] = sizeof(i2c_regs);

  TinyWireS.begin(i2c_regs[0]);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
}

uint8_t idx = 0;

void loop() {
  digitalWrite(LED_1, i2c_regs[idx + 3] & 0x01);
  digitalWrite(LED_2, i2c_regs[idx + 3] & 0x02);
  digitalWrite(LED_3, i2c_regs[idx + 3] & 0x04);
  digitalWrite(LED_4, i2c_regs[idx + 3] & 0x08);
  digitalWrite(LED_5, i2c_regs[idx + 3] & 0x10);
  digitalWrite(LED_6, i2c_regs[idx + 3] & 0x20);
  digitalWrite(LED_7, i2c_regs[idx + 3] & 0x40);
  digitalWrite(LED_8, i2c_regs[idx + 3] & 0x80);
  idx = (idx + 1) % i2c_regs[2];
  
  uint16_t maxCount = i2c_regs[1] * 125;
  uint16_t count = 0;
  do {
    delayMicroseconds(8); // Sleep few micros tjhen check for I2C data again
    count++;

    TinyWireS_stop_check();
    if (write_eeprom) {
      // Store the offset etc configuration registers to EEPROM
      write_eeprom = false;
      regs_eeprom_write();
    }
  } while (count < maxCount);

}
