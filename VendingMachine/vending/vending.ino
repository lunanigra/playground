#define DEFAULT_TIME 2000

void setup() {
  Serial.begin(57600);

  pinMode(13, OUTPUT);

  for (int i = 30; i <= 45; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
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
    for (int i = 30; i <= 45; i++) {
      digitalWrite(i, HIGH);
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

  if (current == "" && action.length() == 2 && action.charAt(0) >= 'A' && action.charAt(0) <= 'E' && action.charAt(1) >= '1' && action.charAt(1) <= '8') {
    current = action;
    onTime = DEFAULT_TIME;
    if (idx != -1) {
      onTime = command.substring(idx + 1).toInt();
    }

    // digitalWrite(action.charAt(0) - 35, LOW);
    digitalWrite(-1 * (action.charAt(0) - 102), LOW);
    digitalWrite(action.charAt(1) - 11, LOW);
    // digitalWrite(-1 * (action.charAt(1) - 94), LOW);
    
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

