#define DEFAULT_TIME 2500

const int ROWS[] = {37, 36, 35, 34, 33, 31};
const int COLS[] = {38, 39, 40, 41, 42, 43, 44, 45};

void setup() {
  Serial.begin(57600);

  pinMode(13, OUTPUT);

  /* for (int i = 30; i <= 45; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  } */
  
  for (int i = 0; i < 6; i++) {
    pinMode(ROWS[i], OUTPUT);
    digitalWrite(ROWS[i], HIGH);
  }
  for (int i = 0; i < 8; i++) {
    pinMode(COLS[i], OUTPUT);
    digitalWrite(COLS[i], HIGH);
  }

  Serial.println("Vending machine ready.");
}

String current = "";
String command;
unsigned long blinkStart;
unsigned long onTime;


void loop() {
  if (readCommand()) {
    parseCommand();
    Serial.println(command);
    command = "";
  }

  if (current != "" && millis() > blinkStart + onTime) {
    for (int i = 0; i < 6; i++) {
    pinMode(ROWS[i], OUTPUT);
      digitalWrite(ROWS[i], HIGH);
    }
    for (int i = 0; i < 8; i++) {
      digitalWrite(COLS[i], HIGH);
    }
    digitalWrite(13, LOW);
    current = "";
    
    sendStatus();
  }
}

void parseCommand() {
  int idx = command.indexOf(",");

  String action = command;
  if (idx != -1) {
    action = command.substring(0, idx);
  }

  if (current == "" && action.length() == 2 && action.charAt(0) >= 'A' && action.charAt(0) <= 'F' && action.charAt(1) >= '1' && action.charAt(1) <= '8') {
    current = action;
    onTime = DEFAULT_TIME;
    if (idx != -1) {
      onTime = command.substring(idx + 1).toInt();
    }

    digitalWrite(ROWS[action.charAt(0) - 65], LOW);
    digitalWrite(COLS[action.charAt(1) - 49], LOW);
    
    digitalWrite(13, HIGH);
    blinkStart = millis();

    sendStatus();
  }
}

int readCommand() {
  char c;
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c != '\n') {
      command += c;
      return false;
    } else return true;
  }
}

void sendStatus() {
  Serial.print("?");
  Serial.println(current);
}

