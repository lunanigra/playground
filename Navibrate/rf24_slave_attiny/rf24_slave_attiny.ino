#include "tinySPI.h"
#include "nRF24L01.h"
#include "RF24.h"

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

char commandVal;             //value sent over via radio
char lastValue;              //stores last state of device (on/off)

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
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);

  // Start listening
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();
}

void loop() {
  // if there is data ready
  if (radio.available()) {
    unsigned long startedTime = millis();
    bool done = false;
    bool timeout = false;
    while (!done) {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&commandVal, sizeof(char));

      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);

      if ((millis() - startedTime) > 1000) {
        timeout = true;
        break;
      }
    }

    if (!timeout) {
      if (commandVal == 'r') {
        blinkRightNextStart = vibrateRightNextStart = millis();
      } else if (commandVal == 'R') {
        blinkRightNowStart = vibrateRightNowStart = millis();
      } else if (commandVal == 'l') {
        blinkLeftNextStart = vibrateLeftNextStart = millis();
      } else if (commandVal == 'L') {
        blinkLeftNowStart = vibrateLeftNowStart = millis();
      } else if (commandVal == 't' || commandVal == 'T') {
        blinkTurnStart = vibrateTurnStart = millis();
      } else if (commandVal == 'x' || commandVal == 'X') {
        blinkXStart = vibrateXStart = millis();
      }
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

void writePinStatus(unsigned long &startedTime, const unsigned int* pattern, size_t size, uint8_t pin1, uint8_t pin2) {
  unsigned long currentTime = millis();
  if (startedTime > 0) {
    int idx = 0;
    unsigned long time = startedTime;
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

