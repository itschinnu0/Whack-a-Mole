int buttonPins[] = { 8, 9, 10, 11, 12, 13 };
int ledPins[] = { 2, 3, 4, 5, 6, 7 };
int redLED = A0;
int greenLED = A1;

int moleCount = 6;
int score = 0;
int missCount = 0;
int currentLevel = 1;
bool gameRunning = false; // The game starts "OFF"
const int rareMoleIndex = 5; // 0-based index of the damaged button mole
const int rareMoleChancePercent = 1; // 1% chance to allow this mole
const unsigned long rareMoleWindowMs = 250UL; // Very short even at level 1
unsigned long gameStartMs = 0;
unsigned long lastLevelCheckMs = 0;
int totalMolesShown = 0;

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
    if (cmd == 'S') { score = 0; missCount = 0; currentLevel = 1; totalMolesShown = 0; gameRunning = true; gameStartMs = millis(); lastLevelCheckMs = gameStartMs; Serial.println("LOG:Game Started"); Serial.println("LOG:Level 1"); }
    if (cmd == 'X') { gameRunning = false; Serial.println("LOG:Game Stopped"); allLedsOff(); }
    if (cmd == 'R') { score = 0; missCount = 0; currentLevel = 1; totalMolesShown = 0; gameStartMs = millis(); lastLevelCheckMs = gameStartMs; Serial.println("LOG:Score Reset"); Serial.println("LOG:Level 1"); }
  }

  if (gameRunning) {
    runGameLogic();
  }
}

void runGameLogic() {
  int mole = chooseMole();
  totalMolesShown++;
  Serial.print("P:"); 
  Serial.println(mole);

  digitalWrite(ledPins[mole], HIGH);
  unsigned long startTime = millis();
  bool hit = false;
  unsigned long moleWindow = getMoleWindowForMole(score, mole);

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
  const unsigned long checkIntervalMs = 15000UL;
  const int minMolesForCheck = 10;
  const int maxLevel = 5;
  const float minAccuracyForLevelUp = 0.75f;
  const float minAccuracyToHold = 0.55f;

  unsigned long now = millis();
  if (now - lastLevelCheckMs < checkIntervalMs) {
    return;
  }

  lastLevelCheckMs = now;

  if (totalMolesShown < minMolesForCheck) {
    return;
  }

  float accuracy = 0.0f;
  if (totalMolesShown > 0) {
    accuracy = (float)score / (float)totalMolesShown;
  }

  int newLevel = currentLevel;
  if (accuracy >= minAccuracyForLevelUp && currentLevel < maxLevel) {
    newLevel = currentLevel + 1;
  } else if (accuracy < minAccuracyToHold && currentLevel > 1) {
    newLevel = currentLevel - 1;
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

int chooseMole() {
  int mole = random(moleCount);
  if (mole == rareMoleIndex) {
    int roll = random(100);
    if (roll >= rareMoleChancePercent) {
      // Pick a different mole if the rare one is not allowed.
      mole = (mole + 1) % moleCount;
    }
  }
  return mole;
}

unsigned long getMoleWindowForMole(int hits, int mole) {
  if (mole == rareMoleIndex) {
    return rareMoleWindowMs;
  }
  return getMoleWindow(hits);
}