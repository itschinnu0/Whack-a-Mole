int buttonPins[] = { 8, 9, 10, 11, 12, 13 };
int ledPins[] = { 2, 3, 4, 5, 6, 7 };
int redLED = A0;
int greenLED = A1;

int moleCount = 6;
int score = 0;
int missCount = 0;
int currentLevel = 1;
bool gameRunning = false; // The game starts "OFF"

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < moleCount; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  randomSeed(analogRead(0));
}

void loop() {
  // Check for commands from the PC
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'S') { score = 0; missCount = 0; currentLevel = 1; gameRunning = true; Serial.println("LOG:Game Started"); Serial.println("LOG:Level 1"); }
    if (cmd == 'X') { gameRunning = false; Serial.println("LOG:Game Stopped"); allLedsOff(); }
    if (cmd == 'R') { score = 0; missCount = 0; currentLevel = 1; Serial.println("LOG:Score Reset"); Serial.println("LOG:Level 1"); }
  }

  if (gameRunning) {
    runGameLogic();
  }
}

void runGameLogic() {
  int mole = random(moleCount);
  Serial.print("P:"); 
  Serial.println(mole);

  digitalWrite(ledPins[mole], HIGH);
  unsigned long startTime = millis();
  bool hit = false;
  unsigned long moleWindow = getMoleWindow(score);

  while (millis() - startTime < moleWindow) {
    // Check for stop command even during the mole window
    if (Serial.available() > 0 && Serial.peek() == 'X') {
      digitalWrite(ledPins[mole], LOW);
      return;
    }

    if (digitalRead(buttonPins[mole]) == LOW) {
      hit = true;
      break;
    }
  }

  digitalWrite(ledPins[mole], LOW);

  if (hit) {
    score++;
    missCount = 0;
    updateLevel();
    Serial.print("H:");
    Serial.println(score);
    digitalWrite(greenLED, HIGH);
    delay(150);
    digitalWrite(greenLED, LOW);
  } else {
    missCount++;
    Serial.println("M");
    digitalWrite(redLED, HIGH);
    delay(150);
    digitalWrite(redLED, LOW);

    if (missCount >= 5) {
      gameRunning = false;
      Serial.println("LOG:Game Over - 5 misses reached");
      allLedsOff();
      return;
    }
  }
  delay(300);
}

unsigned long getMoleWindow(int hits) {
  if (hits < 5) {
    return 2000UL;
  }

  if (hits < 20) {
    return 1200UL;
  }

  return 900UL;
}

void updateLevel() {
  int newLevel = 1;

  if (score >= 20) {
    newLevel = 3;
  } else if (score >= 5) {
    newLevel = 2;
  }

  if (newLevel != currentLevel) {
    currentLevel = newLevel;
    Serial.print("LOG:Level ");
    Serial.println(currentLevel);
  }
}

void allLedsOff() {
  for (int i = 0; i < moleCount; i++) digitalWrite(ledPins[i], LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, LOW);
}