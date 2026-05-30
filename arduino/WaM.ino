/* * Whack-a-Mole: EXPERT REFLEX PRODUCTION BUILD (FINAL CORRECTION)

 * Fixes: Level 15 Unsigned Underflow freeze, Rare Mole high-level scaling floor.

 * Features: Multi-Mole Speed Penalty, Exponential Level Ramping, 2-Mole Max Cap.

 */



// --- HARDWARE CONFIGURATION ---

const int BUTTON_PINS[] = {8, 9, 10, 11, 12, 13}; 

const int LED_PINS[]    = {2, 3, 4, 5, 6, 7}; // Blue Mole LEDs

const int RED_LED       = A0; 

const int GREEN_LED     = A1; 

const int MOLE_COUNT    = 6;



// --- PROTECTED CONSTANTS ---

const int RARE_MOLE_INDEX = 3; 

const int RARE_MOLE_CHANCE = 10;        // 1% chance

const unsigned long RARE_WINDOW = 250; // 250ms base window

const int LIFE_LIMIT = 10;             // Total lives per game



// --- GAME STATE ---

struct Mole {

  bool active = false;

  bool ledOn = false;

  unsigned long startTime = 0;

  unsigned long window = 0;

};



Mole moles[MOLE_COUNT];

bool lastButtonState[MOLE_COUNT];

unsigned long lastButtonTime[MOLE_COUNT]; // Per-button debounce timers

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

  Serial.begin(9600); // Clean serial connection for Android

  for (int i = 0; i < MOLE_COUNT; i++) {

    pinMode(LED_PINS[i], OUTPUT);

    pinMode(BUTTON_PINS[i], INPUT_PULLUP);

    digitalWrite(LED_PINS[i], LOW);

    lastButtonState[i] = HIGH; // High means unpressed (INPUT_PULLUP)

    lastButtonTime[i] = 0;

  }

  pinMode(RED_LED, OUTPUT);

  pinMode(GREEN_LED, OUTPUT);

  randomSeed(analogRead(3));

}



void loop() {

  checkSerialCommands();



  if (gameRunning) {

    manageSpawning();

    // ENGINE ORDER: Check inputs completely before evaluating timeouts

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

  

  // Breather period shrinks drastically after Level 3

  unsigned long breather = (currentLevel < 3) ? 400 : 200;

  if (now - lastResultTime < breather) return; 



  // Hard cap at 2 simultaneous moles max for Level 3+

  int maxActive = (currentLevel >= 3) ? 2 : 1;

  int currentActive = 0;

  for(int i = 0; i < MOLE_COUNT; i++) if(moles[i].active) currentActive++;



  // FIXED: Converted calculation to signed longs to prevent unsigned wrap-around underflow

  long calculatedInterval = 1100L - ((long)currentLevel * 75L);

  unsigned long spawnInterval = (calculatedInterval < 200L) ? 200UL : (unsigned long)calculatedInterval;

  

  if (currentActive < maxActive && (now - lastSpawnTime >= spawnInterval)) {

    spawnMole(currentActive);

    lastSpawnTime = now;

  }

}



void spawnMole(int activeBeforeSpawn) {

  int index = random(MOLE_COUNT);

  

  if (index == RARE_MOLE_INDEX && random(100) >= RARE_MOLE_CHANCE) {

    index = (index + 1) % MOLE_COUNT; 

  }



  if (!moles[index].active) {

    moles[index].active = true;

    moles[index].ledOn = true;

    moles[index].startTime = millis();

    

    // Calculate the dynamic timing window based on current speed tier

    unsigned long normalWindow;

    if (currentLevel == 1) {

      normalWindow = 1500;

    } else if (currentLevel == 2) {

      normalWindow = 1100;

    } else if (currentLevel <= 5) {

      // Levels 3-5: Transition into multi-mole pacing

      normalWindow = 950 - (currentLevel * 100); // L3: 650ms, L4: 550ms, L5: 450ms

    } else if (currentLevel <= 10) {

      // Levels 6-10: Pushing down to the normal human reaction barrier

      normalWindow = 450 - ((currentLevel - 5) * 32); // L10: 290ms

    } else {

      // Levels 11-15+: Exponential wall for elite reaction times (F1 level)

      long calculatedValue = 290 - ((currentLevel - 10) * 24);

      normalWindow = max(170L, calculatedValue); // Level 15 hits extreme 170ms window

    }

    

    // If a mole spawns while another mole is already up, slash its window by 20%

    if (activeBeforeSpawn > 0) {

      normalWindow = (normalWindow * 8) / 10;

    }



    // FIXED: Ensured rare mole is never slower than the accelerated normal window at high levels

    unsigned long rareTargetWindow = (RARE_WINDOW < normalWindow) ? RARE_WINDOW : normalWindow;

    moles[index].window = (index == RARE_MOLE_INDEX) ? rareTargetWindow : normalWindow;

    

    digitalWrite(LED_PINS[index], HIGH);

    Serial.print(F("P:")); Serial.println(index);

  }

}



void checkInputs() {

  unsigned long now = millis();

  for (int i = 0; i < MOLE_COUNT; i++) {

    bool currentState = digitalRead(BUTTON_PINS[i]);



    // State-Stabilized Check: Only track changes that outlast electrical bouncing

    if (currentState != lastButtonState[i]) {

      if (now - lastButtonTime[i] > 35) { // 35ms stable window

        lastButtonTime[i] = now;

        lastButtonState[i] = currentState; 



        if (currentState == LOW) { // Guaranteed clean press event

          if (moles[i].active) {

            handleResult(true, i);

          } else {

            // Anti-slip shielding window

            if (now - lastResultTime >= 200) {

              handleResult(false, -1); 

            }

          }

        }

      }

    }

  }

}



void checkTimeouts() {

  unsigned long now = millis();

  

  // Dynamic Mercy Buffer: Becomes tighter at higher levels to construct a progression wall

  unsigned long mercyBuffer = 50; 

  if (currentLevel >= 11) mercyBuffer = 15;

  else if (currentLevel >= 6) mercyBuffer = 30;



  for (int i = 0; i < MOLE_COUNT; i++) {

    if (moles[i].active) {

      // Step 1: Shut off the physical LED the instant the window expires

      if (moles[i].ledOn && (now - moles[i].startTime >= moles[i].window)) {

        digitalWrite(LED_PINS[i], LOW);

        moles[i].ledOn = false;

      }

      // Step 2: Penalize a miss only after the mercy buffer is exhausted

      if (!moles[i].ledOn && (now - moles[i].startTime >= (moles[i].window + mercyBuffer))) {

        handleResult(false, i); 

      }

    }

  }

}



void handleResult(bool hit, int index) {

  lastResultTime = millis(); 



  if (index != -1) {

    moles[index].active = false;

    moles[index].ledOn = false;

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

    totalMisses++; // Increment total lives spent

    triggerFeedback(RED_LED);

    Serial.print(F("M:")); Serial.println(totalMisses); 

    

    if (totalMisses >= LIFE_LIMIT) startSequence(1); 

  }

}



// --- SEQUENCES (COUNTDOWN, GAME OVER, STOP, RESET) ---



void startSequence(int type) {

  gameRunning = false;

  allLedsOff();

  sequenceType = type;

  lastBlinkToggle = millis();

  

  if (type == 1) {

    sequenceEndTime = millis() + 5000; // 5s Game Over blink

    Serial.println(F("LOG:Game Over"));

  } 

  else if (type == 2) sequenceEndTime = millis() + 1000; // 1s Stop

  else if (type == 3) sequenceEndTime = millis() + 600;  // Reset blink

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



  if (sequenceType == 1) { // 5s Red Game Over

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

    moles[i].ledOn = false;

    lastButtonTime[i] = 0;

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
