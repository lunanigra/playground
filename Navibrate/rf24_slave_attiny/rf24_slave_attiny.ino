#include "tinySPI.h"
#include "nRF24L01.h"
#include "RF24.h"

unsigned long rightNowStart = 0;
unsigned long rightNextStart = 0;
unsigned long leftNowStart = 0;
unsigned long leftNextStart = 0;
unsigned long reverseNowStart = 0;
unsigned long destinationStart = 0;

const unsigned int blinkNowPattern[9] = {500, 200, 500, 200, 500, 200, 500, 200, 500};
const unsigned int blinkNextPattern[9] = {200, 300, 200, 300, 200, 0, 0, 0, 0};
const unsigned int vibrateNowPattern[9] = {200, 200, 200, 200, 200, 200, 2000, 5000, 5000};
const unsigned int vibrateNextPattern[9] = {500, 300, 500, 300, 500, 0, 0, 0, 0};

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins A0 & A1
RF24 radio(PIN_A0, PIN_A1);

// Radio pipe addresses for the 3 nodes to communicate.
const uint64_t pipes[3] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL};

char commandVal;             //value sent over via radio
char lastValue;              //stores last state of device (on/off)

void setup() {
  delay(250);
  pinMode(PIN_B0, OUTPUT);
  pinMode(PIN_B1, OUTPUT);
  delay(250);

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15, 15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8);

  // Open pipes to other nodes for communication

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);

  // Start listening
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();
}

void loop() {
  // if there is data ready
  if (radio.available()) {
    // Dump the payloads until we've gotten everything
    bool done = false;
    while (!done) {
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&commandVal, sizeof(char));

      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);
    }

    if (commandVal == 'r') {
      rightNextStart = millis();
    } else if (commandVal == 'R') {
      rightNowStart = millis();
    } else if (commandVal == 'l') {
      leftNextStart = millis();
    } else if (commandVal == 'L') {
      leftNowStart = millis();
    } else if (commandVal == 't' || commandVal == 'T') {
      reverseNowStart = millis();
    } else if (commandVal == 'x' || commandVal == 'X') {
      destinationStart = millis();
    }

    commandVal = ' ';

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    // radio.write(&got_time, sizeof(unsigned long));
    char respCmd = '$';
    radio.write(&respCmd, sizeof(char));

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }

  handleVibration(rightNowStart, blinkNowPattern, PIN_B0, PIN_B0);
  handleVibration(rightNextStart, blinkNextPattern, PIN_B0, PIN_B0);
  handleVibration(leftNowStart, blinkNowPattern, PIN_B1, PIN_B1);
  handleVibration(leftNextStart, blinkNextPattern, PIN_B1, PIN_B1);
  handleVibration(reverseNowStart, blinkNowPattern, PIN_B0, PIN_B1);
  handleVibration(destinationStart, blinkNextPattern, PIN_B0, PIN_B1);

  delay(20);
}

void handleVibration(unsigned long &start, const unsigned int pattern[], uint8_t pin1, uint8_t pin2) {
  unsigned long currentTime = millis();

  if (start > 0) {
    int idx = 0;
    unsigned long time = start; // pattern[0];
    for (int i = 0; i < 9; i++) {
      if (currentTime > time) idx = i;
      time += pattern[i];
    }
    if (currentTime > time) {
      start = 0;
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    } else {
      digitalWrite(pin1, (idx % 2 == 0) ? HIGH : LOW);
      digitalWrite(pin2, (idx % 2 == 0) ? HIGH : LOW);
    }
  }
}

