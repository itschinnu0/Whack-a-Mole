/* * Whack-a-Mole: FINAL PRODUCTION BUILD
 * Optimized: Hit-Priority, 50ms Mercy Buffer, Life-Based (10 Lives), Max 2 Moles.
 * Stability: Non-blocking state machine, no String overhead, optimized I/O.
 */

// --- HARDWARE CONFIGURATION ---
const int BUTTON_PINS[] = {8, 9, 10, 11, 12, 13}; 
const int LED_PINS[]    = {2, 3, 4, 5, 6, 7}; // Blue Mole LEDs
const int RED_LED       = A0; 
const int GREEN_LED     = A1; 
const int MOLE_COUNT    = 6;

// --- GAME CONSTANTS ---
const int RARE_MOLE_INDEX = 3; 
const int RARE_MOLE_CHANCE = 1;      // 1% chance
const unsigned long RARE_WINDOW = 250; 
const int LIFE_LIMIT = 10;           // Total lives per game
const unsigned long MERCY_BUFFER = 50; // Extra ms after LED off to count hit

// --- GAME STATE ---
struct Mole {
  bool active = false;
  unsigned long startTime = 0;
  unsigned long window = 0;
};

Mole moles[MOLE_COUNT];
bool lastButtonState[MOLE_COUNT];
unsigned long lastResultTime = 0; 
int score = 0;
int totalMisses = 0; 
int currentLevel = 1;
bool gameRunning = false;

// --- TIMING & SEQUENCES ---
unsigned long feedbackEndTime = 0;
unsigned long sequenceEndTime = 0;
unsigned long lastBlinkToggle = 0;
unsigned long lastSpawnTime = 0;
int sequenceType = 0; // 0:None, 1:GameOver, 2:Stop, 3:Reset, 4:Countdown
bool blinkState = false;

void setup() {
  Serial.begin(9600); 
  for (int i = 0; i < MOLE_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    digitalWrite(LED_PINS[i], LOW);
    lastButtonState[i] = HIGH; 
  }
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  randomSeed(analogRead(3)); // Use floating pin for true randomness
}

void loop() {
  checkSerialCommands();

  if (gameRunning) {
    manageSpawning();
    // CRITICAL: Input checked BEFORE Timeouts for Hit-Priority
    checkInputs();
    checkTimeouts();
  } else if (sequenceType != 0) {
    handleSequences(); 
  }
  
  handleFeedbackTimers(); 
}

// --- CORE GAME ENGINE ---

void manageSpawning() {
  unsigned long now = millis();
  
  // Breather period between mole interactions
  unsigned long breather = (currentLevel < 3) ? 400 : 300;
  if (now - lastResultTime < breather) return; 

  // Max 2 moles active simultaneously (Level 3+)
  int maxActive = (currentLevel >= 3) ? 2 : 1;
  int currentActive = 0;
  for(int i = 0; i < MOLE_COUNT; i++) if(moles[i].active) currentActive++;

  // Spawn timing scales with level
  unsigned long spawnInterval = max(300UL, 1000UL - (currentLevel * 70));
  
  if (currentActive < maxActive && (now - lastSpawnTime >= spawnInterval)) {
    spawnMole();
    lastSpawnTime = now;
  }
}

void spawnMole() {
  int index = random(MOLE_COUNT);
  
  // Rare Mole logic
  if (index == RARE_MOLE_INDEX && random(100) >= RARE_MOLE_CHANCE) {
    index = (index + 1) % MOLE_COUNT; 
  }

  if (!moles[index].active) {
    moles[index].active = true;
    moles[index].startTime = millis();
    
    // Window scales from 1.5s down to 500ms
    unsigned long normalWindow;
    if (currentLevel < 3) {
      normalWindow = 1500 - (currentLevel * 250);
    } else {
      normalWindow = max(500UL, 1100UL - (currentLevel * 100));
    }
    
    moles[index].window = (index == RARE_MOLE_INDEX) ? RARE_WINDOW : normalWindow;
    
    digitalWrite(LED_PINS[index], HIGH);
    Serial.print(F("P:")); Serial.println(index);
  }
}

void checkInputs() {
  unsigned long now = millis();
  for (int i = 0; i < MOLE_COUNT; i++) {
    bool currentState = digitalRead(BUTTON_PINS[i]);

    // Detect Falling Edge (Button Press)
    if (currentState == LOW && lastButtonState[i] == HIGH) {
      // 150ms Grace Period to prevent accidental double-hits/noise
      if (now - lastResultTime < 150) { 
          lastButtonState[i] = currentState;
          continue; 
      }

      if (moles[i].active) {
        handleResult(true, i);
      } else {
        handleResult(false, -1); // False hit
      }
    }
    lastButtonState[i] = currentState; 
  }
}

void checkTimeouts() {
  unsigned long now = millis();
  for (int i = 0; i < MOLE_COUNT; i++) {
    // Mercy Buffer (+50ms) applied to timeout check
    if (moles[i].active && (now - moles[i].startTime >= (moles[i].window + MERCY_BUFFER))) {
      handleResult(false, i); 
    }
  }
}

void handleResult(bool hit, int index) {
  lastResultTime = millis(); 

  if (index != -1) {
    moles[index].active = false;
    digitalWrite(LED_PINS[index], LOW);
  }

  if (hit) {
    score++;
    triggerFeedback(GREEN_LED); 
    Serial.print(F("H:")); Serial.println(score);
    
    int newLevel = (score / 10) + 1;
    if (newLevel != currentLevel) {
      currentLevel = newLevel;
      Serial.print(F("LOG:Level ")); Serial.println(currentLevel);
    }
  } else {
    totalMisses++;
    triggerFeedback(RED_LED); 
    Serial.print(F("M:")); Serial.println(totalMisses); 
    
    if (totalMisses >= LIFE_LIMIT) startSequence(1); 
  }
}

// --- SEQUENCES (VISUALS) ---

void startSequence(int type) {
  gameRunning = false;
  allLedsOff();
  sequenceType = type;
  lastBlinkToggle = millis();
  
  if (type == 1) {
    sequenceEndTime = millis() + 5000;
    Serial.println(F("LOG:Game Over"));
  } 
  else if (type == 2) sequenceEndTime = millis() + 1000; // Stop
  else if (type == 3) sequenceEndTime = millis() + 600;  // Reset
  else if (type == 4) sequenceEndTime = millis() + 3100; // Countdown
}

void handleSequences() {
  unsigned long now = millis();
  if (now > sequenceEndTime) {
    bool wasCountdown = (sequenceType == 4);
    sequenceType = 0;
    allLedsOff();
    if (wasCountdown) startGameDirect();
    return;
  }

  if (sequenceType == 1) { // Red Blink
    if (now - lastBlinkToggle >= 250) {
      lastBlinkToggle = now; blinkState = !blinkState;
      digitalWrite(RED_LED, blinkState);
    }
  } 
  else if (sequenceType == 2) { // Stop R+G
    if (now - lastBlinkToggle >= 200) {
      lastBlinkToggle = now; blinkState = !blinkState;
      digitalWrite(RED_LED, blinkState); digitalWrite(GREEN_LED, blinkState);
    }
  }
  else if (sequenceType == 3) { // Reset Blue x2
    if (now - lastBlinkToggle >= 150) {
      lastBlinkToggle = now; blinkState = !blinkState;
      for(int i = 0; i < MOLE_COUNT; i++) digitalWrite(LED_PINS[i], blinkState);
    }
  }
  else if (sequenceType == 4) { // 3-2-1 Countdown
    unsigned long remaining = sequenceEndTime - now;
    allLedsOff();
    if (remaining > 2000) { digitalWrite(LED_PINS[0], HIGH); digitalWrite(LED_PINS[1], HIGH); }
    else if (remaining > 1000) { digitalWrite(LED_PINS[2], HIGH); digitalWrite(LED_PINS[3], HIGH); }
    else if (remaining > 100) { digitalWrite(LED_PINS[4], HIGH); digitalWrite(LED_PINS[5], HIGH); }
  }
}

// --- SYSTEM ---

void checkSerialCommands() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'S') startSequence(4);
    if (cmd == 'X') startSequence(2); 
    if (cmd == 'R') startSequence(3);
  }
}

void startGameDirect() {
  score = 0; 
  totalMisses = 0; 
  currentLevel = 1; 
  gameRunning = true;
  lastResultTime = millis();
  for (int i = 0; i < MOLE_COUNT; i++) {
    lastButtonState[i] = digitalRead(BUTTON_PINS[i]);
    moles[i].active = false;
  }
  allLedsOff();
  Serial.println(F("LOG:Game Started"));
}

void triggerFeedback(int pin) {
  digitalWrite(GREEN_LED, LOW); digitalWrite(RED_LED, LOW); 
  digitalWrite(pin, HIGH);
  feedbackEndTime = millis() + 150; 
}

void handleFeedbackTimers() {
  if (millis() >= feedbackEndTime && sequenceType == 0) {
    digitalWrite(GREEN_LED, LOW); digitalWrite(RED_LED, LOW);
  }
}

void allLedsOff() {
  for (int i = 0; i < MOLE_COUNT; i++) digitalWrite(LED_PINS[i], LOW);
  digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW);
}