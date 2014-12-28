#include "tinySPI.h"
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>

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

// Hardware configuraton: Set up Bluetooth RX, TX
SoftwareSerial Bluetooth(PIN_A2, PIN_A3);

char blueToothVal;           //value sent over via bluetooth
char lastValue;              //stores last state of device (on/off)
char commandVal;

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
  // Open the 'other' pipes for reading, in position #1 (we can have up to 5 pipes open for reading)
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.openReadingPipe(2, pipes[2]);

  // Start listening
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);
  radio.powerUp();
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();

  Bluetooth.begin(9600);
  sendATCommand("AT");
  sendATCommand("AT+NAMENAVIBRATE");
  sendATCommand("AT+PASS000000");
}

bool done = true;

void loop() {
  if (commandVal != ' ') {
    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    bool ok = radio.write(&commandVal, sizeof(char));

    if (ok) {
      commandVal = ' ';
    } else {
      // printf("failed.\n\r");
    }

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while (!radio.available() && !timeout) {
      if (millis() - started_waiting_at > 200) {
        timeout = true;
      }
    }

    // Describe the results
    if (timeout) {
      // printf("Failed, response timed out.\n\r");
    } else {
      // Grab the response, compare, and send to debugging spew
      char respCmd;
      radio.read(&respCmd, sizeof(char));
    }
  }

  // Handle Bluetooth communication
  if (Bluetooth.available()) {
    //if there is data being recieved
    blueToothVal = Bluetooth.read(); //read it

    if (blueToothVal == 'r') {
      Bluetooth.println("OK: r");
      rightNextStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'R') {
      Bluetooth.println("OK: R");
      rightNowStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'l') {
      Bluetooth.println("OK: l");
      leftNextStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'L') {
      Bluetooth.println("OK: L");
      leftNowStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 't' || blueToothVal == 'T') {
      Bluetooth.println("OK: T");
      reverseNowStart = millis();
      commandVal = blueToothVal;
    } else if (blueToothVal == 'x' || blueToothVal == 'X') {
      Bluetooth.println("OK: X");
      destinationStart = millis();
      commandVal = blueToothVal;
    } else {
      Bluetooth.println("Error");
    }
  }

  handleVibration(rightNowStart, blinkNowPattern, PIN_B0, PIN_B0);
  handleVibration(rightNextStart, blinkNextPattern, PIN_B0, PIN_B0);
  handleVibration(leftNowStart, blinkNowPattern, PIN_B1, PIN_B1);
  handleVibration(leftNextStart, blinkNextPattern, PIN_B1, PIN_B1);
  handleVibration(reverseNowStart, blinkNowPattern, PIN_B0, PIN_B1);
  handleVibration(destinationStart, blinkNextPattern, PIN_B0, PIN_B1);

  delay(20);
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

