#include <Arduino.h>
#include <EEPROM.h>

// ---------------- Pin Definitions ----------------
#define VIB_PIN     11
#define PIR_LED     2
#define LED1        3
#define LED2        4
#define LED3        5
#define LED4        6
#define TOUCH_PIN   8
#define PIR_PIN     9
#define BUZZER_PIN  10
#define BUTTON_PIN  12

#define BUZZER_ON  LOW
#define BUZZER_OFF HIGH
#define VIB_TRIGGER HIGH

const int TOTAL_STAGES = 4;
const unsigned long VIBE_DEBOUNCE = 600;
const unsigned long TIMEOUT_MS = 30000;
const unsigned long LONG_PRESS_MS = 3000;

// EEPROM Settings
const int EEPROM_ADDR = 0;
const int CONFIG_ADDR = 100;
const byte CONFIG_SET_VAL = 88;

const int DEFAULT_PASSWORD[TOTAL_STAGES] = {1, 1, 1, 1};

// ---------------- State Management ----------------
enum SystemState {
    WAIT_MOTION,
    WAIT_TOUCH,
    LISTENING_KNOCKS,
    SETUP_MODE,
    WAIT_RESET,
    LOCKOUT
};

SystemState currentState = WAIT_MOTION;

// ---------------- Variables ----------------
int secretPattern[TOTAL_STAGES];
int newPattern[TOTAL_STAGES];
int confirmPattern[TOTAL_STAGES];

bool isConfirming = false;
int currentStage = 0;
int knockCount = 0;
bool lastButtonState;

unsigned long lastVibeTime = 0;
unsigned long lastActionTime = 0;

const int stageLEDs[] = {LED1, LED2, LED3, LED4};
bool stageResult[TOTAL_STAGES];

int failCount = 0;
unsigned long currentPenaltyMs = 0;
unsigned long penaltyStartTime = 0;

// ---------------- Function Declarations ----------------
void loadPassword();
void savePassword();
void clearEEPROM();
void flashLEDs(int count, int speed);
void resetToPhase1();
void checkKnockStage();
void playSuccessBeep();
void playErrorBeep();
void playGrandSuccess();

// ---------------- SETUP ----------------
void setup() {
    Serial.begin(9600);
    delay(500); 
    // Serial.println("\n[DEBUG] --- System Startup Started ---");

    for (int i = 0; i < TOTAL_STAGES; i++) pinMode(stageLEDs[i], OUTPUT);
    pinMode(PIR_LED, OUTPUT);
    
    pinMode(PIR_PIN, INPUT);
    pinMode(TOUCH_PIN, INPUT);
    pinMode(VIB_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_OFF); 
    
    // Serial.println("[DEBUG] Pins configured. Buzzer OFF.");

    // Serial.println("[DEBUG] Loading Password from EEPROM...");
    loadPassword();
    // Serial.print("[DEBUG] Current password = ");
    for (int i = 0; i < TOTAL_STAGES; i++) {
        // Serial.print(secretPattern[i]);
        if (i < TOTAL_STAGES - 1); //Serial.print("-");
    }
    // Serial.println();

    // Serial.println("[DEBUG] Warming up PIR Sensor... Please wait 5 seconds.");
    for(int i = 0; i < 2; i++){
        delay(1000);
        // Serial.print(".");
        digitalWrite(PIR_LED, i % 2); 
    }
    digitalWrite(PIR_LED, LOW);
    // Serial.println("\n[DEBUG] PIR Sensor Stabilized!");

    resetToPhase1();
    lastButtonState = digitalRead(BUTTON_PIN);
    // Serial.println("[DEBUG] --- System Ready. Waiting for Motion (PIR) ---");
}

// ---------------- LOOP ----------------
void loop() {
    unsigned long currentMillis = millis();
    bool buttonRead = digitalRead(BUTTON_PIN);

    if (currentState != WAIT_MOTION && 
        currentState != WAIT_RESET && 
        currentState != LOCKOUT && 
        (currentMillis - lastActionTime > TIMEOUT_MS)) {
        // Serial.println("\n[DEBUG] Timeout! 30 seconds passed without action.");
        resetToPhase1();
    }

    switch (currentState) {
        case WAIT_MOTION:
            if (digitalRead(PIR_PIN) == HIGH) {
                // Serial.println("\n[DEBUG] -> PIR Motion Detected!");
                digitalWrite(PIR_LED, HIGH);
                currentState = WAIT_TOUCH;
                lastActionTime = currentMillis;
                // Serial.println("[DEBUG] -> Moving to Phase 2: Waiting for Touch.");
                delay(500); 
            }
            break;

        case WAIT_TOUCH:
            if (digitalRead(TOUCH_PIN) == HIGH) {
                Serial.println("Taking a photo");
                currentStage = 0;
                knockCount = 0;
                currentState = LISTENING_KNOCKS;
                lastActionTime = currentMillis;
                // Serial.println("[DEBUG] -> Moving to Phase 3: Listening for Knocks.");
                delay(300); 
            }
            break;

        case LISTENING_KNOCKS:
        case SETUP_MODE:
            
            if (currentStage < TOTAL_STAGES) {
                if ((currentMillis / 200) % 2 == 0) {
                    digitalWrite(stageLEDs[currentStage], HIGH);
                } else {
                    digitalWrite(stageLEDs[currentStage], LOW);
                }
            }

            if (currentStage < TOTAL_STAGES &&
                digitalRead(VIB_PIN) == VIB_TRIGGER &&
                (currentMillis - lastVibeTime > VIBE_DEBOUNCE)) {

                knockCount++;
                // Serial.print("[DEBUG] Knock! Current Count: ");
                // Serial.println(knockCount);

                lastVibeTime = currentMillis;
                lastActionTime = currentMillis;

                digitalWrite(BUZZER_PIN, BUZZER_ON);
                delay(20);
                digitalWrite(BUZZER_PIN, BUZZER_OFF);
            }

            if (buttonRead == LOW && lastButtonState == HIGH) {
                // Serial.println("\n[DEBUG] Button Pressed! (Confirming Input)");
                
                if (currentStage < TOTAL_STAGES) {
                    digitalWrite(stageLEDs[currentStage], HIGH);
                }

                if (currentState == SETUP_MODE) {
                    if (!isConfirming) {
                        newPattern[currentStage] = knockCount;
                        // Serial.print("[DEBUG] Setup: Stage "); Serial.print(currentStage+1); Serial.print(" recorded: "); Serial.println(knockCount);
                    } else {
                        confirmPattern[currentStage] = knockCount;
                        // Serial.print("[DEBUG] Confirm: Stage "); Serial.print(currentStage+1); Serial.print(" recorded: "); Serial.println(knockCount);
                    }

                    playSuccessBeep();
                    currentStage++;
                    knockCount = 0;

                    if (currentStage >= TOTAL_STAGES) {
                        if (!isConfirming) {
                            // Serial.println("[DEBUG] Setup Phase 1 Complete. Please enter the same pattern to confirm.");
                            isConfirming = true;
                            currentStage = 0;
                            flashLEDs(3, 100);
                        } else {
                            bool match = true;
                            for (int i = 0; i < TOTAL_STAGES; i++) {
                                if (newPattern[i] != confirmPattern[i]) { match = false; break; }
                            }
                            if (match) {
                                // Serial.println("[DEBUG] Setup SUCCESS! Patterns match. Saving to EEPROM.");
                                for (int i = 0; i < TOTAL_STAGES; i++) secretPattern[i] = newPattern[i];
                                savePassword();
                                flashLEDs(2, 200);
                                resetToPhase1();
                            } else {
                                // Serial.println("[DEBUG] Setup FAILED! Patterns do not match. Try again.");
                                playErrorBeep();
                                loadPassword(); 
                                isConfirming = false;
                                currentStage = 0;
                                flashLEDs(5, 50);
                            }
                        }
                    }
                } else if (currentState == LISTENING_KNOCKS) {
                    checkKnockStage();
                }

                lastActionTime = currentMillis;
                delay(50); 
            }
            break;

        case WAIT_RESET:
            if (buttonRead == LOW && lastButtonState == HIGH) {
                // Serial.println("[DEBUG] Reset button pressed. Restarting sequence.");
                resetToPhase1();
                delay(300);
            }
            break;

        case LOCKOUT:
            if (currentMillis - penaltyStartTime > currentPenaltyMs) {
                // Serial.println("\n[DEBUG] Penalty time finished. System unlocked.");
                resetToPhase1();
            } else {
                if ((currentMillis / 100) % 2 == 0) {
                    for(int i=0; i<TOTAL_STAGES; i++) digitalWrite(stageLEDs[i], HIGH);
                } else {
                    for(int i=0; i<TOTAL_STAGES; i++) digitalWrite(stageLEDs[i], LOW);
                }
            }
            break;
    }

    lastButtonState = buttonRead;
}

// ---------------- Verification Processing ----------------
// ---------------- Verification Processing ----------------
// ---------------- Verification Processing ----------------
void checkKnockStage() {
    stageResult[currentStage] = (knockCount == secretPattern[currentStage]);
    
    // Serial.print("[DEBUG] Checking Stage "); Serial.print(currentStage + 1);
    // Serial.print(": Expected="); Serial.print(secretPattern[currentStage]);
    // Serial.print(", Input="); Serial.print(knockCount);
    // Serial.println(stageResult[currentStage] ? " -> OK!" : " -> NG!");

    digitalWrite(stageLEDs[currentStage], HIGH);
    currentStage++;
    knockCount = 0;

    if (currentStage < 4) {
        playSuccessBeep();
        return;
    }

    bool allCorrect = true;
    for (int i = 0; i < TOTAL_STAGES; i++) {
        if (!stageResult[i]) allCorrect = false;
    }

    if (allCorrect) {
        Serial.println("Open the door");
        playGrandSuccess();
        failCount = 0;
        
        // Serial.println("[DEBUG] Logic: You have 5 seconds to DOUBLE-TAP button to enter SETUP.");
        
        unsigned long windowStart = millis();
        int tapCount = 0;
        bool lastBtn = HIGH;
        bool enterSetup = false;

        while (millis() - windowStart < 5000) {
            bool currentBtn = digitalRead(BUTTON_PIN);
            
            if (currentBtn == LOW && lastBtn == HIGH) {
                tapCount++;
                // Serial.print("[DEBUG] Setup Tap: "); // Serial.println(tapCount);
                
                digitalWrite(BUZZER_PIN, BUZZER_ON);
                delay(50);
                digitalWrite(BUZZER_PIN, BUZZER_OFF);
                
                if (tapCount >= 2) {
                    enterSetup = true;
                    break;
                }
                delay(150);
            }
            lastBtn = currentBtn;
            
            digitalWrite(PIR_LED, (millis() / 250) % 2); 
        }

        if (enterSetup) {
            digitalWrite(PIR_LED, LOW);
            clearEEPROM();
            // Serial.println("\n[DEBUG] >>> ENTERING SETUP MODE <<<");
            currentState = SETUP_MODE;
            currentStage = 0;
            knockCount = 0;
            isConfirming = false;
            lastActionTime = millis();
            flashLEDs(5, 50);
        } else {
            digitalWrite(BUZZER_PIN, BUZZER_ON);
            delay(50);
            digitalWrite(BUZZER_PIN, BUZZER_OFF);
            delay(50);
            digitalWrite(BUZZER_PIN, BUZZER_ON);
            delay(50);
            digitalWrite(BUZZER_PIN, BUZZER_OFF);
            delay(50);
            digitalWrite(BUZZER_PIN, BUZZER_ON);
            delay(50);
            digitalWrite(BUZZER_PIN, BUZZER_OFF);
            resetToPhase1();
        }
 
    } else {
        // Serial.println("\n[DEBUG] *** WRONG PATTERN! ACCESS DENIED ***");
        playErrorBeep();
        Serial.println("Wrong password");
        
        failCount++; 
        if (failCount >= 5){
            currentPenaltyMs = 600000;
            Serial.println("Block");
        }
        currentState = LOCKOUT;
        penaltyStartTime = millis();
    }
}

// ---------------- EEPROM Functions ----------------
void loadPassword() {
    if (EEPROM.read(CONFIG_ADDR) != CONFIG_SET_VAL) {
        // Serial.println("[DEBUG] EEPROM empty/mismatch. Loading default password.");
        for (int i = 0; i < TOTAL_STAGES; i++) secretPattern[i] = DEFAULT_PASSWORD[i];
        savePassword();
    } else {
        // Serial.println("[DEBUG] Loading saved password from EEPROM.");
        for (int i = 0; i < TOTAL_STAGES; i++) secretPattern[i] = EEPROM.read(EEPROM_ADDR + i);
    }
}

void clearEEPROM() {
    EEPROM.update(CONFIG_ADDR, 0xFF);
    // Serial.println("[DEBUG] EEPROM Flag cleared.");
}

void savePassword() {
    for (int i = 0; i < TOTAL_STAGES; i++) EEPROM.update(EEPROM_ADDR + i, (byte)secretPattern[i]);
    EEPROM.update(CONFIG_ADDR, CONFIG_SET_VAL);
    // Serial.println("[DEBUG] Password saved to EEPROM successfully.");
}

// ---------------- Display & Control ----------------
void flashLEDs(int count, int speed) {
    for (int j = 0; j < count; j++) {
        for (int i = 0; i < TOTAL_STAGES; i++) digitalWrite(stageLEDs[i], HIGH);
        delay(speed);
        for (int i = 0; i < TOTAL_STAGES; i++) digitalWrite(stageLEDs[i], LOW);
        delay(speed);
    }
}

void resetToPhase1() {
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(PIR_LED, LOW);
    for (int i = 0; i < TOTAL_STAGES; i++) digitalWrite(stageLEDs[i], LOW);
    digitalWrite(BUZZER_PIN, BUZZER_OFF); 
    
    currentStage = 0;
    knockCount = 0;
    isConfirming = false;
    currentState = WAIT_MOTION;
    // Serial.println("[DEBUG] System Reset -> Waiting for Motion (PIR)");
}

// ---------------- Buzzer Control ----------------
void playSuccessBeep() {
  tone(BUZZER_PIN, 1500, 120);
  delay(120);
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH);
}

void playErrorBeep() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 400, 200);
    delay(250);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void playLockoutBeep() {
  tone(BUZZER_PIN, 200, 200);
  delay(200);

  tone(BUZZER_PIN, 200, 200);
  delay(200);

  tone(BUZZER_PIN, 200, 500);
  delay(600);

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH);
}

void playGrandSuccess() {
  tone(BUZZER_PIN, 1500, 200);
  delay(200);

  tone(BUZZER_PIN, 1000, 200);
  delay(200);

  tone(BUZZER_PIN, 1250, 200);
  delay(200);

  tone(BUZZER_PIN, 2000, 500);
  delay(500);

  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH);
}
