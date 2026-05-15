/*
 * Whack-a-Mole: Production Edition
 * Optimized for Android USB-OTG Communication
 */

// --- CONFIGURATION ---
const int BUTTON_PINS[] = {8, 9, 10, 11, 12, 13};
const int LED_PINS[]    = {2, 3, 4, 5, 6, 7};
const int RED_LED       = A0;
const int GREEN_LED     = A1;
const int MOLE_COUNT    = 6;

// --- GAME CONSTANTS ---
const int RARE_MOLE_INDEX = 5; 
const int RARE_MOLE_CHANCE = 1; // 1%
const unsigned long RARE_WINDOW = 250;

// --- STATE VARIABLES ---
int score = 0;
int missCount = 0;
int currentLevel = 1;
bool gameRunning = false;
int activeMole = -1;
int totalMolesShown = 0;

unsigned long moleStartTime = 0;
unsigned long moleWindow = 0;
unsigned long lastLevelCheckMs = 0;
unsigned long feedbackEndTime = 0;

void setup() {
  Serial.begin(9600); // Keep at 9600 to match your C# legacy or move to 115200 for speed
  
  for (int i = 0; i < MOLE_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  randomSeed(analogRead(3)); // Use an unconnected analog pin for better randomness
  Serial.println("LOG:System Ready");
}

void loop() {
  checkSerialCommands();

  if (gameRunning) {
    manageGameState();
    checkPhysicalInput();
  }
  
  handleFeedbackTimers();
}

// --- CORE LOGIC ---

void checkSerialCommands() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'S': startNewGame(); break;
      case 'X': stopGame(); break;
      case 'R': resetStats(); break;
    }
  }
}

void manageGameState() {
  // If no mole is active, pick one
  if (activeMole == -1) {
    spawnMole();
  } else {
    // Check if mole timed out (Miss)
    if (millis() - moleStartTime >= moleWindow) {
      handleResult(false);
    }
  }
  
  updateLevelLogic();
}

void checkPhysicalInput() {
  if (activeMole != -1) {
    if (digitalRead(BUTTON_PINS[activeMole]) == LOW) {
      handleResult(true);
    }
  }
}

void spawnMole() {
  activeMole = chooseMole();
  totalMolesShown++;
  moleStartTime = millis();
  moleWindow = getMoleWindowForMole(score, activeMole);
  
  digitalWrite(LED_PINS[activeMole], HIGH);
  Serial.print("P:");
  Serial.println(activeMole);
}

void handleResult(bool hit) {
  // Turn off the mole LED immediately
  if (activeMole != -1) digitalWrite(LED_PINS[activeMole], LOW);
  
  if (hit) {
    score++;
    missCount = 0;
    Serial.print("H:");
    Serial.println(score);
    triggerFeedback(GREEN_LED);
  } else {
    missCount++;
    Serial.println("M");
    triggerFeedback(RED_LED);
    
    if (missCount >= 5) {
      Serial.println("LOG:Game Over - 5 misses");
      stopGame();
      return;
    }
  }
  
  activeMole = -1; // Reset for next spawn
  moleStartTime = millis() + 300; // Small delay before next mole (non-blocking)
}

// --- HELPERS ---

int chooseMole() {
  int mole = random(MOLE_COUNT);
  if (mole == RARE_MOLE_INDEX && random(100) >= RARE_MOLE_CHANCE) {
    mole = (mole + 1) % MOLE_COUNT;
  }
  return mole;
}

unsigned long getMoleWindowForMole(int hits, int mole) {
  if (mole == RARE_MOLE_INDEX) return RARE_WINDOW;
  if (hits < 5) return 2000UL;
  if (hits < 20) return 1200UL;
  return 900UL;
}

void triggerFeedback(int pin) {
  digitalWrite(pin, HIGH);
  feedbackEndTime = millis() + 150;
}

void handleFeedbackTimers() {
  if (millis() >= feedbackEndTime) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
  }
}

void startNewGame() {
  resetStats();
  gameRunning = true;
  Serial.println("LOG:Game Started");
}

void stopGame() {
  gameRunning = false;
  activeMole = -1;
  allLedsOff();
  Serial.println("LOG:Game Stopped");
}

void resetStats() {
  score = 0;
  missCount = 0;
  currentLevel = 1;
  totalMolesShown = 0;
  lastLevelCheckMs = millis();
}

void allLedsOff() {
  for (int i = 0; i < MOLE_COUNT; i++) digitalWrite(LED_PINS[i], LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}

void updateLevelLogic() {
  unsigned long now = millis();
  if (now - lastLevelCheckMs < 15000UL || totalMolesShown < 10) return;

  float accuracy = (float)score / (float)totalMolesShown;
  int newLevel = currentLevel;

  if (accuracy >= 0.75f && currentLevel < 5) newLevel++;
  else if (accuracy < 0.55f && currentLevel > 1) newLevel--;

  if (newLevel != currentLevel) {
    currentLevel = newLevel;
    Serial.print("LOG:Level ");
    Serial.println(currentLevel);
  }
  lastLevelCheckMs = now;
}