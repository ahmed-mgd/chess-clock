#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Pin definitions
const int B_BUTTON = 14;       // Black player button
const int W_BUTTON = 26;       // White player button
const int CONTROL_BUTTON = 27; // Control button for start/pause
const int BUZZER_PIN = 32;     // Buzzer pin (updated from 15 to 32)

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Time options in seconds
const int TIME_OPTIONS[] = {120, 600, 1800, 3600}; // 2min, 10min, 30min, 60min
const int NUM_OPTIONS = 4;
int currentTimeOption = 1; // Default to 10 minutes (index 1)

// Game state variables
unsigned long blackTime;
unsigned long whiteTime;
unsigned long lastUpdateTime;
bool isWhiteTurn = true; // White starts
bool isRunning = false;
bool isSetup = true;

// Buzzer helper functions
void playStartupBeep() {
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 50; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(500);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(500);
    }
    delay(100);
  }
}

void playWarningBeep() {
  for (int i = 0; i < 50; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(500);
  }
}

void playFinalSecondsBeep() {
  for (int i = 0; i < 100; i++) { // Longer, higher-pitched beep
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(250);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(250);
  }
}

void setup() {
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize buttons with internal pullup resistors
  pinMode(B_BUTTON, INPUT_PULLUP);
  pinMode(W_BUTTON, INPUT_PULLUP);
  pinMode(CONTROL_BUTTON, INPUT_PULLUP);
  
  // Initialize times
  resetTimes();
  
  // Show welcome message
  lcd.print("Chess Clock");
  lcd.setCursor(0, 1);
  lcd.print("Select time...");
  delay(2000);
}

void loop() {
  static bool wasSetup = true;
  static bool hasOneMinuteWarning = false;
  static bool hasThirtySecondWarning = false;
  static unsigned long lastFinalSecondBeep = 0;
  
  if (isSetup) {
    handleSetup();
    
    // Reset warning flags when in setup mode
    hasOneMinuteWarning = false;
    hasThirtySecondWarning = false;
    lastFinalSecondBeep = 0;
  } else {
    // Check if just started the game
    if (wasSetup && !isSetup) {
      playStartupBeep(); // Beep twice when game starts
    }
    
    handleGame();
    
    // Warning beeps and final seconds management
    if (isRunning) {
      unsigned long currentTime = millis();
      unsigned long* currentPlayerTime = isWhiteTurn ? &whiteTime : &blackTime;
      
      // 1-minute warning (only once)
      if (*currentPlayerTime <= 60000 && !hasOneMinuteWarning) {
        playWarningBeep();
        hasOneMinuteWarning = true;
      }
      
      // 30-second warning (only once)
      if (*currentPlayerTime <= 30000 && !hasThirtySecondWarning) {
        playWarningBeep();
        hasThirtySecondWarning = true;
      }
      
      // Final 5 seconds beeps
      if (*currentPlayerTime <= 5000) {
        // Beep once per second in the last 5 seconds
        for (int i = 5; i > 0; i--) {
          if (*currentPlayerTime <= (unsigned long)i * 1000 && 
              currentTime - lastFinalSecondBeep >= 1000) {
            playFinalSecondsBeep();
            lastFinalSecondBeep = currentTime;
          }
        }
      }
    }
  }
  
  wasSetup = isSetup;
}

void handleSetup() {
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(TIME_OPTIONS[currentTimeOption] / 60);
  lcd.print(" min   "); // Ensure to overwrite extra characters
  
  // Handle button presses for time selection
  if (digitalRead(B_BUTTON) == LOW) {
    currentTimeOption = (currentTimeOption + 1) % NUM_OPTIONS;
    delay(200); // Debounce
  }
  if (digitalRead(W_BUTTON) == LOW) {
    currentTimeOption = (currentTimeOption - 1 + NUM_OPTIONS) % NUM_OPTIONS;
    delay(200); // Debounce
  }
  if (digitalRead(CONTROL_BUTTON) == LOW) {
    isSetup = false;
    resetTimes();
    lcd.clear(); // Clear only when exiting setup
    delay(200); // Debounce
  }
}

void handleGame() {
  static bool lastTurn = isWhiteTurn; // Track the last turn state
  
  if (digitalRead(CONTROL_BUTTON) == LOW) {
    isRunning = !isRunning;
    lastUpdateTime = millis();
    delay(200); // Debounce
  }
  
  if (isRunning) {
    // Update time for current player
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastUpdateTime;
    
    if (isWhiteTurn) {
      whiteTime = (elapsed >= whiteTime) ? 0 : whiteTime - elapsed;
    } else {
      blackTime = (elapsed >= blackTime) ? 0 : blackTime - elapsed;
    }
    
    lastUpdateTime = currentTime;
    
    // Handle player switches
    if (isWhiteTurn && digitalRead(W_BUTTON) == LOW) {
      isWhiteTurn = false;
      delay(200); // Debounce
    } else if (!isWhiteTurn && digitalRead(B_BUTTON) == LOW) {
      isWhiteTurn = true;
      delay(200); // Debounce
    }
  }
  
  // Update display if the turn changes
  if (isWhiteTurn != lastTurn) {
    lastTurn = isWhiteTurn;
    updateTurnDisplay();
  }
  
  // Update times
  displayTimes();
  
  // Check for game end
  if (whiteTime == 0 || blackTime == 0) {
    gameOver();
  }
}

void displayTimes() {
  // First line: Black's time
  lcd.setCursor(0, 0);
  lcd.print("B: ");
  printFormattedTime(blackTime);
  lcd.print("     "); // Clear remaining characters

  // Second line: White's time
  lcd.setCursor(0, 1);
  lcd.print("W: ");
  printFormattedTime(whiteTime);
  lcd.print("     "); // Clear remaining characters
}

void updateTurnDisplay() {
  // Clear current turn marker
  lcd.setCursor(15, 0);
  lcd.print(" ");
  lcd.setCursor(15, 1);
  lcd.print(" ");
  
  // Set new turn marker
  lcd.setCursor(15, isWhiteTurn ? 1 : 0);
  lcd.print("*");
}

void printFormattedTime(unsigned long timeInMs) {
  int seconds = timeInMs / 1000;
  int minutes = seconds / 60;
  seconds %= 60;
  
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
}

void resetTimes() {
  blackTime = TIME_OPTIONS[currentTimeOption] * 1000; // Convert to milliseconds
  whiteTime = TIME_OPTIONS[currentTimeOption] * 1000;
  lastUpdateTime = millis();
}

void gameOver() {
  isRunning = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game Over!");
  lcd.setCursor(0, 1);
  if (whiteTime == 0) {
    lcd.print("Black Wins!");
  } else {
    lcd.print("White Wins!");
  }
  
  // Wait for control button to restart
  while (digitalRead(CONTROL_BUTTON) == HIGH) {
    delay(50);
  }
  
  isSetup = true;
  resetTimes();
}