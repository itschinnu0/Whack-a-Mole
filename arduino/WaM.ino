/* * Whack-a-Mole: PURE LEGENDARY EDITION
 * No warm-ups. No mercy. 
 */

// --- CONFIGURATION ---
const int BUTTON_PINS[] = {8, 9, 10, 11, 12, 13}; 
const int LED_PINS[]    = {2, 3, 4, 5, 6, 7};     
const int RED_LED       = A0;                     
const int GREEN_LED     = A1;                     
const int MOLE_COUNT    = 6;                      

// --- YOUR PROTECTED CONSTANTS ---
const int RARE_MOLE_INDEX = 5; 
const int RARE_MOLE_CHANCE = 1; 
const unsigned long RARE_WINDOW = 250; 

// --- LEGENDARY TUNING ---
const int MAX_ACTIVE_MOLES = 2; // Chaos enabled immediately
const int MISS_LIMIT = 5;
const unsigned long BASE_WINDOW = 650; // Fast from the start
const unsigned long MIN_WINDOW = 400;  // Absolute floor for normal moles

// --- GAME STATE ---
struct Mole {
  bool active = false;
  unsigned long startTime = 0;
  unsigned long window = 0;
};

Mole moles[MOLE_COUNT];
int score = 0;
int missCount = 0;
bool gameRunning = false;
unsigned long lastSpawnTime = 0;
unsigned long feedbackEndTime = 0;
unsigned long spawnDelay = 400; 

void setup() {
  Serial.begin(9600); // Stable baud rate for Android [cite: 76]
  for (int i = 0; i < MOLE_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  randomSeed(analogRead(3)); 
}

void loop() {
  checkSerial();

  if (gameRunning) {
    manageSpawning();
    checkInputs();
    checkTimeouts();
  }
  
  handleFeedbackTimers();
}

// --- CORE ENGINE ---

void manageSpawning() {
  unsigned long now = millis();
  int currentActive = 0;
  for(int i=0; i<MOLE_COUNT; i++) if(moles[i].active) currentActive++;

  // Legendary Mode: Can have 2 moles active at once from Level 1
  if (currentActive < MAX_ACTIVE_MOLES && (now - lastSpawnTime >= spawnDelay)) {
    spawnMole();
    lastSpawnTime = now;
  }
}

void spawnMole() {
  int index = random(MOLE_COUNT);

  // Apply Rare Mole Logic [cite: 35, 36, 37]
  if (index == RARE_MOLE_INDEX && random(100) >= RARE_MOLE_CHANCE) {
    index = (index + 1) % MOLE_COUNT; 
  }

  if (!moles[index].active) {
    moles[index].active = true;
    moles[index].startTime = millis();
    
    // Determine Window: Rare vs Legendary Normal
    moles[index].window = (index == RARE_MOLE_INDEX) ? RARE_WINDOW : calculateHardWindow();
    
    digitalWrite(LED_PINS[index], HIGH);
    Serial.print("P:"); 
    Serial.println(index); 
  }
}

void checkInputs() {
  for (int i = 0; i < MOLE_COUNT; i++) {
    if (digitalRead(BUTTON_PINS[i]) == LOW) {
      if (moles[i].active) {
        handleResult(true, i);
      } else {
        // FALSE-HIT PENALTY 
        handleResult(false, -1); 
        delay(50); // Small debounce
      }
    }
  }
}

void checkTimeouts() {
  for (int i = 0; i < MOLE_COUNT; i++) {
    if (moles[i].active && (millis() - moles[i].startTime >= moles[i].window)) {
      handleResult(false, i); 
    }
  }
}

void handleResult(bool hit, int index) {
  if (index != -1) {
    moles[index].active = false;
    digitalWrite(LED_PINS[index], LOW);
  }

  if (hit) {
    score++;
    missCount = 0;
    triggerFeedback(GREEN_LED);
    Serial.print("H:");
    Serial.println(score);
    // Increase spawn speed slightly per hit
    spawnDelay = max(200UL, 400UL - (score * 4));
  } else {
    missCount++;
    triggerFeedback(RED_LED);
    Serial.println("M");
    if (missCount >= MISS_LIMIT) stopGame("Game Over");
  }
}

// --- HELPERS ---

unsigned long calculateHardWindow() {
  // Legendary scaling: Starts fast, gets faster
  unsigned long current = BASE_WINDOW - (score * 5);
  return max(current, MIN_WINDOW);
}

void checkSerial() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'S' || cmd == 'R') startGame();
    if (cmd == 'X') stopGame("Stopped");
  }
}

void startGame() {
  score = 0;
  missCount = 0;
  spawnDelay = 400;
  gameRunning = true;
  for(int i=0; i<MOLE_COUNT; i++) moles[i].active = false;
  allLedsOff();
  Serial.println("LOG:Legendary Mode Started");
}

void stopGame(String reason) {
  gameRunning = false;
  allLedsOff();
  Serial.print("LOG:");
  Serial.println(reason);
}

void triggerFeedback(int pin) {
  digitalWrite(pin, HIGH);
  feedbackEndTime = millis() + 100;
}

void handleFeedbackTimers() {
  if (millis() >= feedbackEndTime) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
  }
}

void allLedsOff() {
  for (int i = 0; i < MOLE_COUNT; i++) digitalWrite(LED_PINS[i], LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}