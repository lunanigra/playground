#include "tinySPI.h"
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>

unsigned long blinkRightNowStart = 0;
unsigned long blinkRightNextStart = 0;
unsigned long blinkLeftNowStart = 0;
unsigned long blinkLeftNextStart = 0;
unsigned long blinkTurnStart = 0;
unsigned long blinkXStart = 0;

unsigned long vibrateRightNowStart = 0;
unsigned long vibrateRightNextStart = 0;
unsigned long vibrateLeftNowStart = 0;
unsigned long vibrateLeftNextStart = 0;
unsigned long vibrateTurnStart = 0;
unsigned long vibrateXStart = 0;

const unsigned int blinkNowPattern[] = {500, 200, 500, 200, 500, 200, 500, 200, 500};
const unsigned int blinkNextPattern[] = {150, 250, 150, 250, 150, 250, 150, 250, 150};
const unsigned int vibrateNowPattern[] = {2500};
const unsigned int vibrateNextPattern[] = {400, 1000, 400};

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins A0 & A1
RF24 radio(PIN_A0, PIN_A1);

// Radio pipe addresses for the 3 nodes to communicate.
const uint64_t pipes[3] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL};

// Hardware configuraton: Set up Bluetooth RX, TX
SoftwareSerial Bluetooth(PIN_A2, PIN_A3);

char blueToothVal;           //value sent over via bluetooth
char lastValue;              //stores last state of device (on/off)
char commandVal;

void setup() {
  pinMode(PIN_B0, OUTPUT);
  pinMode(PIN_B1, OUTPUT);
  pinMode(PIN_B2, OUTPUT);
  pinMode(PIN_A7, OUTPUT);
  delay(100);

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(sizeof(char));

  // Open pipes to other nodes for communication

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipes for reading, in position #1 (we can have up to 5 pipes open for reading)
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.openReadingPipe(2, pipes[2]);

  // Start listening
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();

  Bluetooth.begin(9600);
  sendATCommand("AT");
  sendATCommand("AT+NAMENAVIBRATE");
  sendATCommand("AT+PASS000000");
  // sendATCommand("AT+ADVI1");
}

bool done = true;

void loop() {
  if (commandVal != ' ') {
    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    // bool ok = radio.write(&commandVal, sizeof(char), true);
    bool ok = radio.write(&commandVal, sizeof(char));

    if (ok) {
      commandVal = ' ';
    } else {
      delay(20);
    }

    // Now, continue listening
    radio.startListening();
  }

  // Handle Bluetooth communication
  if (Bluetooth.available()) {
    // if there is data being recieved
    blueToothVal = Bluetooth.read(); //read it

    if (blueToothVal == 'r') {
      Bluetooth.println("OK: r");
      blinkRightNextStart = vibrateRightNextStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'R') {
      Bluetooth.println("OK: R");
      blinkRightNowStart = vibrateRightNowStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'l') {
      Bluetooth.println("OK: l");
      blinkLeftNextStart = vibrateLeftNextStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'L') {
      Bluetooth.println("OK: L");
      blinkLeftNowStart = vibrateLeftNowStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 't' || blueToothVal == 'T') {
      Bluetooth.println("OK: T");
      blinkTurnStart = vibrateTurnStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'x' || blueToothVal == 'X') {
      Bluetooth.println("OK: X");
      blinkXStart = vibrateXStart = millis();
      commandVal = blueToothVal;
    } else {
      Bluetooth.println("Error");
    }
  }

  writePinStatus(blinkRightNowStart, blinkNowPattern, sizeof(blinkNowPattern) / sizeof(unsigned int), PIN_B0, PIN_B0);
  writePinStatus(blinkRightNextStart, blinkNextPattern, sizeof(blinkNextPattern) / sizeof(unsigned int), PIN_B0, PIN_B0);
  writePinStatus(blinkLeftNowStart, blinkNowPattern, sizeof(blinkNowPattern) / sizeof(unsigned int), PIN_B1, PIN_B1);
  writePinStatus(blinkLeftNextStart, blinkNextPattern, sizeof(blinkNextPattern) / sizeof(unsigned int), PIN_B1, PIN_B1);
  writePinStatus(blinkXStart, blinkNowPattern, sizeof(blinkNowPattern) / sizeof(unsigned int), PIN_B0, PIN_B1);
  writePinStatus(blinkTurnStart, blinkNextPattern, sizeof(blinkNextPattern) / sizeof(unsigned int), PIN_B0, PIN_B1);

  writePinStatus(vibrateRightNowStart, vibrateNowPattern, sizeof(vibrateNowPattern) / sizeof(unsigned int), PIN_B2, PIN_B2);
  writePinStatus(vibrateRightNextStart, vibrateNextPattern, sizeof(vibrateNextPattern) / sizeof(unsigned int), PIN_B2, PIN_B2);
  writePinStatus(vibrateLeftNowStart, vibrateNowPattern, sizeof(vibrateNowPattern) / sizeof(unsigned int), PIN_A7, PIN_A7);
  writePinStatus(vibrateLeftNextStart, vibrateNextPattern, sizeof(vibrateNextPattern) / sizeof(unsigned int), PIN_A7, PIN_A7);
  writePinStatus(vibrateXStart, vibrateNowPattern, sizeof(vibrateNowPattern) / sizeof(unsigned int), PIN_B2, PIN_A7);
  writePinStatus(vibrateTurnStart, vibrateNextPattern, sizeof(vibrateNextPattern) / sizeof(unsigned int), PIN_B2, PIN_A7);

  // delay(20);
}

void sendATCommand(const String& command) {
  Bluetooth.print(command);
  boolean done = false;
  unsigned long time = millis();
  while (!done && (millis() - time < 5000)) {
    if (Bluetooth.available()) {
      blueToothVal = Bluetooth.read();
      if (blueToothVal == 'O') {
        done = true;
        time = millis();
        while (millis() - time < 1000) {
          if (Bluetooth.available()) {
            blueToothVal = Bluetooth.read();
          }
        }
      }
    }
  }
}

void writePinStatus(unsigned long &startedTime, const unsigned int* pattern, size_t size, uint8_t pin1, uint8_t pin2) {
  unsigned long currentTime = millis();
  if (startedTime > 0) {
    int idx = 0;
    unsigned long time = startedTime; // pattern[0];
    for (int i = 0; i < size; i++) {
      if (currentTime > time) idx = i;
      time += pattern[i];
    }
    if (currentTime > time) {
      startedTime = 0;
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    } else {
      digitalWrite(pin1, (idx % 2 == 0) ? HIGH : LOW);
      digitalWrite(pin2, (idx % 2 == 0) ? HIGH : LOW);
    }
  }
}

