#include <LedControl.h>
#include <LiquidCrystal.h>

// pins
const int MATRIX_DIN  = 12;
const int MATRIX_CLK  = 11;
const int MATRIX_LOAD = 10;

const int JOY_X_PIN   = A0;
const int JOY_Y_PIN   = A1;
const int JOY_BTN_PIN = A2;

const int DRAW_BTN_PIN  = A3;
const int CLEAR_BTN_PIN = A4;
const int PAUSE_BTN_PIN = A5; 

const int BUZZER_PIN = 13;

const int ULTRASONIC_TRIG_PIN = 2;
const int ULTRASONIC_ECHO_PIN = 3;


const int LCD_RS = 9;
const int LCD_EN = 8;
const int LCD_D4 = 7;
const int LCD_D5 = 6;
const int LCD_D6 = 5;
const int LCD_D7 = 4;

// objects
LedControl   matrix(MATRIX_DIN, MATRIX_CLK, MATRIX_LOAD, 1);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// intro skip flag
const bool SKIP_INTRO = false;

// game states
enum GameState {
  STATE_INTRO,
  STATE_TUTORIAL_INTRO,
  STATE_MENU,
  STATE_EXPLORE,
  STATE_COMBAT,
  STATE_PAUSE,
  STATE_PAUSE_SPELLS,
  STATE_WIN,
  STATE_LOSE
};

GameState gameState = STATE_INTRO;
GameState savedGameState;    

// menu states
enum MenuState {
  MENU_MAIN,
  MENU_TUTORIAL,
  MENU_SPELLS,
  MENU_CREDITS
};

MenuState menuState = MENU_MAIN;

// menu data
const int optionNumber = 4;
int mainMenuIndex = 0;

int tutorialPage = 0;
const int TUTORIAL_PAGES = 5;

int spellsPage = 0;
const int SPELLS_PAGES = 3;

// pause menu
int pauseMenuIndex = 0; // 0..3

// map 16x16
const int MAP_W = 16;
const int MAP_H = 16;

// 1 = wall, 0 = floor
uint8_t rawMap[MAP_H][MAP_W] = {
  //  0 1 2 3 4 5 6 7   8 9 10 11 12 13 14 15
  {1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1},
  {1,0,0,0,1,0,0,0,  0,0,1,0,0,0,0,1},
  {1,0,1,0,1,0,1,1,  1,0,1,0,1,1,0,1},
  {1,0,1,0,0,0,0,0,  1,0,0,0,0,1,0,1},
  {1,0,1,1,1,1,1,0,  1,1,1,1,0,1,0,1},
  {1,0,0,0,0,0,1,0,  0,0,0,1,0,1,0,1},
  {1,1,1,1,1,0,1,1,  1,1,0,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,  0,1,0,0,0,1,0,1},

  {1,0,1,0,1,1,1,1,  0,1,1,1,0,1,0,1},
  {1,0,1,0,0,0,0,1,  0,0,0,1,0,0,0,1},
  {1,0,1,1,1,1,0,1,  1,1,0,1,1,1,0,1},
  {1,0,0,0,0,1,0,0,  0,1,0,0,0,1,0,1},
  {1,1,1,1,0,1,1,1,  0,1,1,1,0,1,0,1},
  {1,0,0,1,0,0,0,1,  0,0,0,1,0,0,0,1},
  {1,0,0,0,0,1,0,0,  0,1,0,0,0,1,0,1},
  {1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1}
};

bool mapData[MAP_H][MAP_W];

// viewport 8x8
int viewX = 0;
int viewY = 0;

// tutorial vars

bool tutorialCursorMoved = false;
unsigned long tutorialHintBlinkLast = 0;
bool tutorialHintOn = true;
const unsigned long TUTORIAL_HINT_BLINK_INTERVAL = 400;

// explore player
int playerGX = 1;
int playerGY = 1;
int prevPlayerGX = 1;
int prevPlayerGY = 1;
bool playerVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 300;

bool escapeCooldownActive = false;
unsigned long escapeCooldownStart = 0;
const unsigned long ESCAPE_COOLDOWN_DURATION = 1500; // 1.5s


// multiple moles
const int MOLE_COUNT = 5;
int moleGX[MOLE_COUNT];
int moleGY[MOLE_COUNT];
bool moleAliveArr[MOLE_COUNT];
bool moleVisible = true;
unsigned long lastMoleBlinkTime = 0;
const unsigned long MOLE_BLINK_INTERVAL = 400;

int currentMoleIndex = -1;

// items
const int POTION_COUNT = 3;
int potionGX[POTION_COUNT];
int potionGY[POTION_COUNT];
bool potionActive[POTION_COUNT];

bool potionVisible = true;
unsigned long lastPotionBlinkTime = 0;
const unsigned long POTION_BLINK_INTERVAL = 900;

bool potionMessageActive = false;
unsigned long potionMessageStart = 0;
const unsigned long POTION_MSG_DURATION = 3000;

// joystick
const int JOY_CENTER_MIN = 400;
const int JOY_CENTER_MAX = 624;

unsigned long lastMoveTime = 0;
const unsigned long MOVE_INTERVAL = 150;

// ultrasonic vars
const int minDistance = 6;

// combat
int cursorX = 3;
int cursorY = 3;
bool cursorVisible = true;
unsigned long lastCursorBlinkTime = 0;
const unsigned long CURSOR_BLINK_INTERVAL = 250;

bool combatPixels[8][8];
bool lastJoyBtnState = HIGH;
bool lastPauseBtnState = HIGH;

// combat vars
const int MAGE_MAX_HP = 20;
const int MOLE_MAX_HP = 18;
int mageHP = MAGE_MAX_HP;
int moleHP = MOLE_MAX_HP;
int mageShield = 0;
int moleShield = 0;
int moleStunTurns = 0;

int basePixelsPerTurn = 8;
int extraPixelsAfterFirstKill = 4;
int pixelsPerTurn = 8;
int pixelsUsedThisTurn = 0;
bool firstMoleKilled = false;

bool isMageTurn = true;
const int MOLE_ATTACK_CHANCE = 70;

bool escapePossibleThisCombat = false;
bool escapeRequested = false;

// combat turn flow
enum MoleAction {
  MOLE_ACT_NONE,
  MOLE_ACT_ATTACK,
  MOLE_ACT_SHIELD
};

MoleAction lastMoleAction = MOLE_ACT_NONE;

bool enemyTurnActive = false;
unsigned long enemyTurnStartTime = 0;
const unsigned long ENEMY_TURN_DURATION = 2000;

bool enemyMessageActive = false;
unsigned long enemyMessageStartTime = 0;
const unsigned long ENEMY_MESSAGE_DURATION = 600;


// spells
const uint8_t ATTACK_SPELL[8] = {
  0b00000000,
  0b00000000,
  0b00100100,
  0b00011000,
  0b00011000,
  0b00100100,
  0b00000000,
  0b00000000
};

const uint8_t SHIELD_SPELL[8] = {
  0b00000000,
  0b00000000,
  0b00011000,
  0b00111100,
  0b00111100,
  0b00011000,
  0b00000000,
  0b00000000
};

const uint8_t STUN_SPELL[8] = {
  0b00000000,
  0b00000000,
  0b00011100,
  0b00101010,
  0b01010100,
  0b00111000,
  0b00000000,
  0b00000000
};

const int ATTACK_ANIM_DELAY = 120;
const int SHIELD_ANIM_DELAY = 120;

// game menu matrix
const uint64_t IMAGES[] = {
  0x1c2222ff7e1c1830
};
const int IMAGES_LEN = sizeof(IMAGES) / 8;

long score = 0;

// intro helpers
int introPage = 0;
unsigned long introLastChange = 0;
const unsigned long INTRO_DELAY = 3000;
const unsigned long INTRO_DELAY_LONG = 5000;
bool tutorialAttackHintShown = false;
bool firstAttackDone = false;

// fwd decl
void setupMatrix();
void setupLCD();
void setupJoystick();
void initMap();

void drawHudStatic();

void loopIntro();
void loopTutorialIntro();

void loopMenu();
void loopMenuMain();
void loopMenuTutorial();
void loopMenuSpells();
void loopMenuCredits();
void printMenuOptionLabel(int idx);

void loopExplore();
void loopCombat();
void loopPause();
void loopPauseSpells();
void loopWin();
void loopLose();

void drawViewport();
void clearMatrix();
void drawPlayerAndMoles();
void updatePlayerBlink();
void updateMoleBlink();
void updatePotionBlink();
void handleJoystickMoveExplore();
bool isWallGlobal(int gx, int gy);
void updateLCDGameExplore();
bool isNearMole();
void updateViewport();
void checkPotionPickup();

void handleJoystickMoveCombat();
void drawCombatLayer();
void updateCursorBlink();
void handleCombatButtons(bool drawPressed, bool clearPressed);
void clearCombatPattern();
void enterCombatMode();
void exitCombatModeWin();
void exitCombatModeLose();
void updateLCDGameCombat();

bool isJoystickButtonPressedEdge();
bool isPauseButtonPressedEdge();

bool spellMatchesBitmap(const uint8_t pattern[8]);
bool isSpellAttack();
bool isSpellShield();

void applyMageAttack();
void applyMageShield();
void moleTurn();
void resetCombatTurn();

void flashAttackAnimation();
void shieldAnimation();

bool anyMoleAlive();

// setup
void setup() {
  setupMatrix();
  setupLCD();
  setupJoystick();
  initMap();
  setupUltrasonicSensor();
  pinMode(BUZZER_PIN, OUTPUT);

  playerGX = 1;
  playerGY = 1;
  prevPlayerGX = 1;
  prevPlayerGY = 1;
  score = 0;

  gameState = STATE_MENU;   // start in menu
  menuState = MENU_MAIN;
  mainMenuIndex = 0;

  randomSeed(analogRead(A5));
}

// loop
void loop() {
  switch (gameState) {
    case STATE_INTRO:          loopIntro();         break;
    case STATE_TUTORIAL_INTRO: loopTutorialIntro(); break;
    case STATE_MENU:           loopMenu();          break;
    case STATE_EXPLORE:        loopExplore();       break;
    case STATE_COMBAT:         loopCombat();        break;
    case STATE_PAUSE:          loopPause();         break;
    case STATE_PAUSE_SPELLS:   loopPauseSpells();   break;
    case STATE_WIN:            loopWin();           break;
    case STATE_LOSE:           loopLose();          break;
  }
}

void setupMatrix() {
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 8);
  matrix.clearDisplay(0);
}

void setupLCD() {
  lcd.begin(16, 2);
  lcd.clear();

  byte glyph0[8] = { B00000,B00001,B00011,B00111,B01111,B00110,B00100,B00011 };
  byte glyph1[8] = { B11000,B10000,B10000,B11100,B11110,B01100,B00100,B11001 };
  byte glyph2[8] = { B00001,B00011,B00111,B00101,B00101,B00011,B00111,B01111 };
  byte glyph3[8] = { B10011,B10010,B11110,B10011,B10001,B11011,B11010,B11010 };

  byte name0x14[8] = { B00000,B00000,B00011,B00100,B01011,B11110,B11000,B01001 };
  byte name1x15[8] = { B00100,B11000,B10000,B11100,B01110,B10010,B11100,B01100 };
  byte name0x15[8] = { B00000,B00000,B11000,B00100,B11010,B01111,B00011,B10010 };
  byte name1x14[8] = { B00100,B01011,B01101,B00111,B00010,B00001,B00111,B00110 };

  byte name1x7[8]  = { B00000,B11111,B01110,B00100,B01110,B11111,B11111,B01110 };

  lcd.createChar(0, glyph0);
  lcd.createChar(1, glyph1);
  lcd.createChar(2, glyph2);
  lcd.createChar(3, glyph3);
  lcd.createChar(4, name0x14);  
  lcd.createChar(5, name0x15);
  lcd.createChar(6, name1x14);
  lcd.createChar(7, name1x15);

  drawHudStatic();
}

void setupJoystick() {
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);
  pinMode(DRAW_BTN_PIN, INPUT_PULLUP);
  pinMode(CLEAR_BTN_PIN, INPUT_PULLUP);
  pinMode(PAUSE_BTN_PIN, INPUT_PULLUP);
}

void setupUltrasonicSensor() {
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
}

void drawHudStatic() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.setCursor(1, 0);
  lcd.write(byte(1));
  lcd.setCursor(0, 1);
  lcd.write(byte(2));
  lcd.setCursor(1, 1);
  lcd.write(byte(3));
}

void drawImageOnMatrix(uint64_t img) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      int bitIndex = y * 8 + x;
      bool on = (img >> bitIndex) & 0x1;
      matrix.setLed(0, y, x, on);
    }
  }
}

void initMap() {
  for (int y = 0; y < MAP_H; y++) {
    for (int x = 0; x < MAP_W; x++) {
      mapData[y][x] = (rawMap[y][x] == 1);
    }
  }

  moleGX[0] = 3;  moleGY[0] = 3;
  moleGX[1] = 7;  moleGY[1] = 7;
  moleGX[2] = 13; moleGY[2] = 13;
  moleGX[3] = 2;   moleGY[3] = 5;
  moleGX[4] = 10;  moleGY[4] = 9;

  potionGX[0] = 5;  potionGY[0] = 3;
  potionGX[1] = 8;  potionGY[1] = 9;
  potionGX[2] = 13; potionGY[2] = 2;

  for (int i = 0; i < POTION_COUNT; i++) {
    potionActive[i] = true;
  }
  for (int i = 0; i < MOLE_COUNT; i++) {
    moleAliveArr[i] = true;
    mapData[moleGY[i]][moleGX[i]] = false;
  }

  viewX = 0;
  viewY = 0;
}

void playAttackSound() {
  tone(BUZZER_PIN, 700, 120);  
}

void playShieldSound() {
  tone(BUZZER_PIN, 400, 150);
}

void playStunSuccessSound() {
  tone(BUZZER_PIN, 1000, 80);
  delay(90);
  tone(BUZZER_PIN, 1300, 80);
}

void playStunFailSound() {
  tone(BUZZER_PIN, 200, 200);
}


// ---------- INTRO ----------

void loopIntro() {
  unsigned long now = millis();

  if (isJoystickButtonPressedEdge()) {
    gameState = STATE_TUTORIAL_INTRO;
    tutorialAttackHintShown = false;
    firstAttackDone = false;
    clearMatrix();
    clearCombatPattern();
    return;
  }

  if (introPage == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You wake up in");
    lcd.setCursor(0, 1);
    lcd.print("a twisted place");
    introLastChange = now;
    introPage = 1;
    return;
  }

  if (introPage == 1 && now - introLastChange > INTRO_DELAY) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Head is heavy,");
    lcd.setCursor(0, 1);
    lcd.print("mind full of fog");
    introLastChange = now;
    introPage = 2;
    return;
  }

  if (introPage == 2 && now - introLastChange > INTRO_DELAY) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No memory of");
    lcd.setCursor(0, 1);
    lcd.print("what came before");
    introLastChange = now;
    introPage = 3;
    return;
  }

  if (introPage == 3 && now - introLastChange > INTRO_DELAY) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("A book and a");
    lcd.setCursor(0, 1);
    lcd.print("dim staff wait");
    introLastChange = now;
    introPage = 4;
    return;
  }

  if (introPage == 4 && now - introLastChange > INTRO_DELAY) {
    gameState = STATE_TUTORIAL_INTRO;
    tutorialAttackHintShown = false;
    firstAttackDone = false;
    clearMatrix();
    clearCombatPattern();
  }
}

enum TutorialIntroState {
  TUT_INTRO_TEXT1,
  TUT_INTRO_TEXT2,
  TUT_INTRO_DRAWING,
  TUT_INTRO_AFTER_SUCCESS_POWER1,
  TUT_INTRO_AFTER_SUCCESS_POWER2,
  TUT_INTRO_FAIL_MSG,
  TUT_INTRO_DONE
};

TutorialIntroState tutorialIntroState = TUT_INTRO_TEXT1;
unsigned long tutorialIntroStateStart = 0;

// first spell tutorial
void loopTutorialIntro() {
  unsigned long now = millis();

  // global exit to menu (only outside drawing)
  if (isJoystickButtonPressedEdge() && tutorialIntroState != TUT_INTRO_DRAWING) {
    gameState = STATE_MENU;
    return;
  }

  // state: first text
  if (tutorialIntroState == TUT_INTRO_TEXT1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You grab staff");
    lcd.setCursor(0, 1);
    lcd.print("and open book");
    tutorialIntroStateStart = now;
    tutorialIntroState = TUT_INTRO_TEXT2;
    return;
  }

  // state: second text
  if (tutorialIntroState == TUT_INTRO_TEXT2) {
    if (now - tutorialIntroStateStart >= INTRO_DELAY) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try a symbol");
      lcd.setCursor(0, 1);
      lcd.print("on matrix");
      tutorialIntroStateStart = now;
      tutorialIntroState = TUT_INTRO_DRAWING;

      tutorialAttackHintShown = true;
      firstAttackDone = false;
      tutorialCursorMoved = false;
      clearMatrix();
      clearCombatPattern();
      cursorX = 3;
      cursorY = 3;
      cursorVisible = true;
      lastCursorBlinkTime = now;
      tutorialHintBlinkLast = now;
      tutorialHintOn = true;
    }
    return;
  }

  // state: drawing and hint
  if (tutorialIntroState == TUT_INTRO_DRAWING) {
    int xVal = analogRead(JOY_X_PIN);
    int yVal = analogRead(JOY_Y_PIN);

    if (!tutorialCursorMoved) {
      // until player moves joystick: blink ATTACK_SPELL on matrix
      bool moved =
        (xVal < JOY_CENTER_MIN || xVal > JOY_CENTER_MAX ||
         yVal < JOY_CENTER_MIN || yVal > JOY_CENTER_MAX);
      if (moved) {
        tutorialCursorMoved = true;
        clearMatrix();
      } else {
        if (now - tutorialHintBlinkLast >= TUTORIAL_HINT_BLINK_INTERVAL) {
          tutorialHintBlinkLast = now;
          tutorialHintOn = !tutorialHintOn;
          clearMatrix();
          if (tutorialHintOn) {
            for (int y = 0; y < 8; y++) {
              for (int x = 0; x < 8; x++) {
                bool bitOn = (ATTACK_SPELL[y] >> (7 - x)) & 0x01;
                if (bitOn) matrix.setLed(0, y, x, true);
              }
            }
          }
        }
        return;
      }
    }

    // after first joystick move: normal drawing mode
    handleJoystickMoveCombat();

    bool drawPressed  = (digitalRead(DRAW_BTN_PIN)  == LOW);
    bool clearPressed = (digitalRead(CLEAR_BTN_PIN) == LOW);
    handleCombatButtons(drawPressed, clearPressed);
    updateCursorBlink();

    clearMatrix();
    drawCombatLayer();

    if (isJoystickButtonPressedEdge()) {
      if (isSpellAttack()) {
        firstAttackDone = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("You feel power");
        lcd.setCursor(0, 1);
        lcd.print("through veins");
        tutorialIntroStateStart = now;
        tutorialIntroState = TUT_INTRO_AFTER_SUCCESS_POWER1;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("The mark fails");
        lcd.setCursor(0, 1);
        lcd.print("Try again");
        tutorialIntroStateStart = now;
        tutorialIntroState = TUT_INTRO_FAIL_MSG;
      }
    }
    return;
  }

  // success text 1
  if (tutorialIntroState == TUT_INTRO_AFTER_SUCCESS_POWER1) {
    if (now - tutorialIntroStateStart >= INTRO_DELAY_LONG) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("The power...");
      tutorialIntroStateStart = now;
      tutorialIntroState = TUT_INTRO_AFTER_SUCCESS_POWER2;
    }
    return;
  }

  // success text 2 then enter game
  if (tutorialIntroState == TUT_INTRO_AFTER_SUCCESS_POWER2) {
    if (now - tutorialIntroStateStart >= INTRO_DELAY_LONG) {
      lcd.clear();
      drawHudStatic();
      gameState = STATE_EXPLORE;
      playerGX = 1;
      playerGY = 1;
      prevPlayerGX = playerGX;
      prevPlayerGY = playerGY;
      score = 0;
      lastBlinkTime = millis();
      lastMoleBlinkTime = millis();
      mageHP = MAGE_MAX_HP;
      moleHP = MOLE_MAX_HP;
      mageShield = 0;
      moleShield = 0;
      pixelsPerTurn = basePixelsPerTurn + (firstMoleKilled ? extraPixelsAfterFirstKill : 0);
      updateViewport();
      clearCombatPattern();
      clearMatrix();
      tutorialIntroState = TUT_INTRO_DONE;
    }
    return;
  }

  // fail message, then retry drawing
  if (tutorialIntroState == TUT_INTRO_FAIL_MSG) {
    if (now - tutorialIntroStateStart >= 2000) {
      lcd.clear();
      tutorialIntroState = TUT_INTRO_DRAWING;
      clearCombatPattern();
      clearMatrix();
      tutorialCursorMoved = false;
      tutorialHintBlinkLast = now;
      tutorialHintOn = true;
    }
    return;
  }
}

// MENU

void loopMenu() {
  switch (menuState) {
    case MENU_MAIN:     loopMenuMain();     break;
    case MENU_TUTORIAL: loopMenuTutorial(); break;
    case MENU_SPELLS:   loopMenuSpells();   break;
    case MENU_CREDITS:  loopMenuCredits();  break;
  }
}

void printMenuOptionLabel(int idx) {
  int normalized = ((idx % optionNumber) + optionNumber) % optionNumber;
  switch (normalized) {
    case 0: lcd.print("Start       ");   break;
    case 1: lcd.print("Tutorial    ");   break;
    case 2: lcd.print("Spells      ");   break;
    case 3: lcd.print("Credits     ");   break;
  }
}

void loopMenuMain() {
  int yVal = analogRead(JOY_Y_PIN);

  static unsigned long lastNavTime = 0;
  const unsigned long NAV_INTERVAL = 200;
  unsigned long now = millis();

  if (now - lastNavTime > NAV_INTERVAL) {
    if (yVal > JOY_CENTER_MAX) {
      mainMenuIndex--;
      lastNavTime = now;
    } else if (yVal < JOY_CENTER_MIN) {
      mainMenuIndex++;
      lastNavTime = now;
    }
    mainMenuIndex = ((mainMenuIndex % optionNumber) + optionNumber) % optionNumber;
  }

  lcd.setCursor(0, 0);
  lcd.print("Molemage Trials");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  printMenuOptionLabel(mainMenuIndex);
  drawImageOnMatrix(IMAGES[0]);
  if (isJoystickButtonPressedEdge()) {
    int sel = ((mainMenuIndex % optionNumber) + optionNumber) % optionNumber;
    switch (sel) {
      case 0:
        if (SKIP_INTRO) {
          drawHudStatic();
          gameState = STATE_EXPLORE;
          playerGX = 1;
          playerGY = 1;
          prevPlayerGX = playerGX;
          prevPlayerGY = playerGY;
          score = 0;
          lastBlinkTime = millis();
          lastMoleBlinkTime = millis();
          mageHP = MAGE_MAX_HP;
          moleHP = MOLE_MAX_HP;
          mageShield = 0;
          moleShield = 0;
          pixelsPerTurn = basePixelsPerTurn + (firstMoleKilled ? extraPixelsAfterFirstKill : 0);
          updateViewport();
        } else {
          gameState = STATE_INTRO;
          introPage = 0;
          introLastChange = millis();
        }
        break;

      case 1:
        menuState = MENU_TUTORIAL;
        tutorialPage = 0;
        lcd.clear();
        break;

      case 2:
        menuState = MENU_SPELLS;
        spellsPage = 0;
        lcd.clear();
        clearMatrix();
        break;

      case 3:
        menuState = MENU_CREDITS;
        lcd.clear();
        break;
    }
  }
}

void renderTutorialPage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tutorial");
  lcd.setCursor(0, 1);
  switch (tutorialPage) {
    case 0: lcd.print("Use joystick   "); break;
    case 1: lcd.print("to move        "); break;
    case 2: lcd.print("MID=draw pix"); break;
    case 3: lcd.print("LEFT=erase Sw=OK"); break;
    case 4: lcd.print("RIGHT=pause menu"); break;
  }
}

void loopMenuTutorial() {
  int yVal = analogRead(JOY_Y_PIN);

  static unsigned long lastNavTime = 0;
  const unsigned long NAV_INTERVAL = 200;
  unsigned long now = millis();

  if (now - lastNavTime > NAV_INTERVAL) {
    if (yVal > JOY_CENTER_MAX) {
      if (tutorialPage > 0) tutorialPage--;
      lastNavTime = now;
    } else if (yVal < JOY_CENTER_MIN) {
      if (tutorialPage < TUTORIAL_PAGES - 1) tutorialPage++;
      lastNavTime = now;
    }
  }

  renderTutorialPage();

  if (isJoystickButtonPressedEdge()) {
    menuState = MENU_MAIN;
    lcd.clear();
  }
}

void renderSpellOnMatrix(int idx) {
  clearMatrix();
  if (idx == 0) {
    // attack spell
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        bool bitOn = (ATTACK_SPELL[y] >> (7 - x)) & 0x01;
        if (bitOn) matrix.setLed(0, y, x, true);
      }
    }
  } else if (idx == 1) {
    // shield spell
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        bool bitOn = (SHIELD_SPELL[y] >> (7 - x)) & 0x01;
        if (bitOn) matrix.setLed(0, y, x, true);
      }
    }
  } else if (idx == 2) {
    // stun spell
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        bool bitOn = (STUN_SPELL[y] >> (7 - x)) & 0x01;
        if (bitOn) matrix.setLed(0, y, x, true);
      }
    }
  }
}



void renderSpellsPage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Spells");
  lcd.setCursor(0, 1);
  switch (spellsPage) {
    case 0: lcd.print("Basic Attack   "); break;
    case 1: lcd.print("Shield Spell   "); break;
    case 2: lcd.print("Stun Spell     "); break;
  }
  renderSpellOnMatrix(spellsPage);
}

void loopMenuSpells() {
  int yVal = analogRead(JOY_Y_PIN);

  static unsigned long lastNavTime = 0;
  const unsigned long NAV_INTERVAL = 200;
  unsigned long now = millis();

  if (now - lastNavTime > NAV_INTERVAL) {
    if (yVal > JOY_CENTER_MAX) {
      if (spellsPage > 0) spellsPage--;
      lastNavTime = now;
    } else if (yVal < JOY_CENTER_MIN) {
      if (spellsPage < SPELLS_PAGES - 1) spellsPage++;
      lastNavTime = now;
    }
  }
  renderSpellsPage();

  if (isJoystickButtonPressedEdge()) {
    clearMatrix();
    menuState = MENU_MAIN;
    lcd.clear();
  }
}

void loopMenuCredits() {
  lcd.setCursor(0, 0);
  lcd.print("Molemage Trials");
  lcd.setCursor(0, 1);
  lcd.print("made by Octavian");

  if (isJoystickButtonPressedEdge()) {
    menuState = MENU_MAIN;
    lcd.clear();
  }
}

// EXPLORE

void loopExplore() {
  if (isPauseButtonPressedEdge()) {
    savedGameState = STATE_EXPLORE;
    gameState = STATE_PAUSE;
    pauseMenuIndex = 0;
    return;
  }

  handleJoystickMoveExplore();
  updateViewport();
  updatePlayerBlink();
  updateMoleBlink();
  updatePotionBlink();
  checkPotionPickup();

  clearMatrix();
  drawViewport();
  drawPlayerAndMoles();

    // only allow combat if no cooldown
  if (!escapeCooldownActive && isNearMole()) {
    enterCombatMode();
    return;
  }

  if (escapeCooldownActive) {
    if (millis() - escapeCooldownStart >= ESCAPE_COOLDOWN_DURATION) {
      escapeCooldownActive = false;
    }
  }

    if (!potionMessageActive) {
    updateLCDGameExplore();
  }
}

void drawViewport() {
  for (int sy = 0; sy < 8; sy++) {
    int gy = viewY + sy;
    if (gy < 0 || gy >= MAP_H) continue;
    for (int sx = 0; sx < 8; sx++) {
      int gx = viewX + sx;
      if (gx < 0 || gx >= MAP_W) continue;
      if (mapData[gy][gx]) {
        matrix.setLed(0, sy, sx, true);
      }
    }
  }
}

void clearMatrix() {
  matrix.clearDisplay(0);
}

void drawPlayerAndMoles() {
  int pSX = playerGX - viewX;
  int pSY = playerGY - viewY;
  if (playerVisible && pSX >= 0 && pSX < 8 && pSY >= 0 && pSY < 8) {
    matrix.setLed(0, pSY, pSX, true);
  }

  for (int i = 0; i < MOLE_COUNT; i++) {
    if (!moleAliveArr[i]) continue;
    int mSX = moleGX[i] - viewX;
    int mSY = moleGY[i] - viewY;
    if (mSX >= 0 && mSX < 8 && mSY >= 0 && mSY < 8 && moleVisible) {
      matrix.setLed(0, mSY, mSX, true);
    }
  }

  for (int i = 0; i < POTION_COUNT; i++) {
    if (!potionActive[i]) continue;
    int sX = potionGX[i] - viewX;
    int sY = potionGY[i] - viewY;
    if (sX >= 0 && sX < 8 && sY >= 0 && sY < 8 && potionVisible) {
      matrix.setLed(0, sY, sX, true);
    }
  }
}

void updatePlayerBlink() {
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime = now;
    playerVisible = !playerVisible;
  }
}

void updateMoleBlink() {
  unsigned long now = millis();
  if (now - lastMoleBlinkTime >= MOLE_BLINK_INTERVAL) {
    lastMoleBlinkTime = now;
    moleVisible = !moleVisible;
  }
}

void updatePotionBlink() {
  unsigned long now = millis();
  if (now - lastPotionBlinkTime >= POTION_BLINK_INTERVAL) {
    lastPotionBlinkTime = now;
    potionVisible = !potionVisible;
  }
}

bool isNearMole() {
  currentMoleIndex = -1;
  for (int i = 0; i < MOLE_COUNT; i++) {
    if (!moleAliveArr[i]) continue;
    int dx = playerGX - moleGX[i];
    int dy = playerGY - moleGY[i];
    if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
      if (!(dx == 0 && dy == 0)) {
        currentMoleIndex = i;
        return true;
      }
    }
  }
  return false;
}

void updateViewport() {
  viewX = playerGX - 4;
  viewY = playerGY - 4;
  if (viewX < 0) viewX = 0;
  if (viewY < 0) viewY = 0;
  if (viewX > MAP_W - 8) viewX = MAP_W - 8;
  if (viewY > MAP_H - 8) viewY = MAP_H - 8;
}

void handleJoystickMoveExplore() {
  unsigned long now = millis();
  if (now - lastMoveTime < MOVE_INTERVAL) return;

  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);

  int newGX = playerGX;
  int newGY = playerGY;

  if (xVal < JOY_CENTER_MIN) newGX = playerGX - 1;
  else if (xVal > JOY_CENTER_MAX) newGX = playerGX + 1;

  if (yVal < JOY_CENTER_MIN) newGY = playerGY + 1;
  else if (yVal > JOY_CENTER_MAX) newGY = playerGY - 1;

  bool moved = false;

  if (!isWallGlobal(newGX, playerGY) && newGX != playerGX) {
    prevPlayerGX = playerGX;
    prevPlayerGY = playerGY;
    playerGX = newGX;
    moved = true;
  }
  if (!isWallGlobal(playerGX, newGY) && newGY != playerGY) {
    prevPlayerGX = playerGX;
    prevPlayerGY = playerGY;
    playerGY = newGY;
    moved = true;
  }

  if (moved) {
    lastMoveTime = now;
    score++;
  }
}

void checkPotionPickup() {
  unsigned long now = millis();

  if (potionMessageActive) {
    if (now - potionMessageStart >= POTION_MSG_DURATION) {
      potionMessageActive = false;
      drawHudStatic();
      updateLCDGameExplore();
    }
    return;
  }

  for (int i = 0; i < POTION_COUNT; i++) {
    if (!potionActive[i]) continue;
    if (playerGX == potionGX[i] && playerGY == potionGY[i]) {
      potionActive[i] = false;

      mageHP += 8;
      if (mageHP > MAGE_MAX_HP) {
        mageHP = MAGE_MAX_HP;
      }

      lcd.clear();
      lcd.print(" heal potion");
      lcd.setCursor(0, 1);
      lcd.print("picked & used");
      potionMessageActive = true;
      potionMessageStart = now;
      break;
    }
  }
}


bool isWallGlobal(int gx, int gy) {
  if (gx < 0 || gx >= MAP_W || gy < 0 || gy >= MAP_H) return true;
  return mapData[gy][gx];
}

void updateLCDGameExplore() {
  int kills = getMolesKilled();

  lcd.setCursor(2, 0);
  lcd.print("HP:");
  lcd.print(mageHP);
  lcd.print("   ");

  lcd.setCursor(2, 1);
  lcd.print("Mole:");
  lcd.print(kills);
  lcd.print("/");
  lcd.print(MOLE_COUNT);
  lcd.print("   ");
}


// COMBAT

void enterCombatMode() {
  if (currentMoleIndex < 0 || currentMoleIndex >= MOLE_COUNT) return;

  gameState = STATE_COMBAT;
  clearCombatPattern();
  cursorX = 3;
  cursorY = 3;
  cursorVisible = true;
  lastCursorBlinkTime = millis();

  moleHP = MOLE_MAX_HP;
  mageShield = 0;
  moleShield = 0;
  isMageTurn = true;
  escapePossibleThisCombat = true;  // first round
  escapeRequested = false;
  pixelsPerTurn = basePixelsPerTurn + (firstMoleKilled ? extraPixelsAfterFirstKill : 0);
  pixelsUsedThisTurn = 0;
}

void exitCombatModeWin() {
  if (currentMoleIndex >= 0 && currentMoleIndex < MOLE_COUNT) {
    moleAliveArr[currentMoleIndex] = false;
  }

  firstMoleKilled = true;
  pixelsPerTurn += 4;

  if (!anyMoleAlive()) {
    gameState = STATE_WIN;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All moles");
    lcd.setCursor(0, 1);
    lcd.print("defeated! GG");
  } else {
    gameState = STATE_EXPLORE;
    clearCombatPattern();
    clearMatrix();
    drawHudStatic();
    updateViewport();
  }
}

void performCombatEscape() {
  // reset mole stats for next combat
  moleHP = MOLE_MAX_HP;
  moleShield = 0;
  moleStunTurns = 0;

  gameState = STATE_EXPLORE;
  clearCombatPattern();
  clearMatrix();
  drawHudStatic();
  updateViewport();

  escapeCooldownActive = true;
  escapeCooldownStart = millis();
  escapeRequested = false;
  escapePossibleThisCombat = false;
}



void exitCombatModeLose() {
  gameState = STATE_LOSE;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("You died...");
  lcd.setCursor(0, 1);
  lcd.print("Press SW btn");
}

void loopCombat() {
  // pause
  if (isPauseButtonPressedEdge()) {
    savedGameState = STATE_COMBAT;
    gameState = STATE_PAUSE;
    pauseMenuIndex = 0;
    return;
  }

  updateAttackAnimation();
  updateShieldAnimation();

  // escape sensor check (only during first round, one-shot)
  if (escapePossibleThisCombat && !escapeRequested) {
    long d = readDistanceCm();
    if (d < minDistance) {
      escapeRequested = true;
    }
  }

  if (enemyTurnActive) {
  unsigned long now = millis();

  // phase 1: wait for enemy action time, then show message once
  if (!enemyMessageActive) {
    if (now - enemyTurnStartTime >= ENEMY_TURN_DURATION) {
      enemyMessageActive = true;
      enemyMessageStartTime = now;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mole turn:");

      lcd.setCursor(0, 1);
      if (moleStunTurns > 0 && lastMoleAction == MOLE_ACT_NONE) {
        lcd.print("Mole is stunned");
      } else if (lastMoleAction == MOLE_ACT_ATTACK) {
        lcd.print("Mole attacked");
      } else if (lastMoleAction == MOLE_ACT_SHIELD) {
        lcd.print("Mole shielded");
      } else {
        lcd.print("Mole did nothing");
      }
    }
    return;
  }

  // phase 2: keep message for ENEMY_MESSAGE_DURATION, then return to mage
  if (enemyMessageActive) {
    if (now - enemyMessageStartTime >= ENEMY_MESSAGE_DURATION) {
      enemyTurnActive = false;
      enemyMessageActive = false;

      if (mageHP <= 0) {
        exitCombatModeLose();
        return;
      }
      if (moleHP <= 0) {
        exitCombatModeWin();
        return;
      }
      
      // if escape was requested during first round escape now
      if (escapeRequested && escapePossibleThisCombat) {
        escapePossibleThisCombat = false;
        escapeRequested = false;
        performCombatEscape();
        return;
      }

      // after first full round escape no longer allowed
      escapePossibleThisCombat = false;

      updateLCDGameCombat();
      isMageTurn = true;
    }
    return;
  }

  return;
}




  if (isMageTurn) {
    handleJoystickMoveCombat();

    bool drawPressed  = (digitalRead(DRAW_BTN_PIN)  == LOW);
    bool clearPressed = (digitalRead(CLEAR_BTN_PIN) == LOW);
    handleCombatButtons(drawPressed, clearPressed);
    updateCursorBlink();

    clearMatrix();
    drawCombatLayer();

    updateLCDGameCombat();

    if (isJoystickButtonPressedEdge()) {
      bool usedValidSpell = false;
      bool enemyTurnAlreadyStarted = false;

      // ATTACK
      if (isSpellAttack()) {
        applyMageAttack();
        usedValidSpell = true;
        playAttackSound();

      // SHIELD
      } else if (isSpellShield()) {
        applyMageShield();
        usedValidSpell = true;
        playShieldSound();

      // STUN
      } else if (isSpellStun()) {
        int r = random(0, 100);
        MoleAction plannedAction;
        if (r < MOLE_ATTACK_CHANCE) {
          plannedAction = MOLE_ACT_ATTACK;
        } else {
          plannedAction = MOLE_ACT_SHIELD;
        }

        if (plannedAction == MOLE_ACT_SHIELD) {
          playStunSuccessSound();
          moleShield = 0;
          moleStunTurns = 2;
          lastMoleAction = MOLE_ACT_NONE;

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Stun success");
          lcd.setCursor(0, 1);
          lcd.print("Mole stunned");

          usedValidSpell = true;

          enemyTurnActive = true;
          enemyTurnStartTime = millis(); // reuse duration to keep text on screen
          isMageTurn = false;
          enemyTurnAlreadyStarted = true;
        } else {
          // ignore stun as enemy intent is to attack
          playStunFailSound();
          usedValidSpell = true;

          moleTurn();  // apply damage to enemy
          enemyTurnActive = true;
          enemyTurnStartTime = millis();
          isMageTurn = false;
          enemyTurnAlreadyStarted = true;
        }

      } else {
        // pattern invalid
      }

      if (usedValidSpell) {
        if (moleHP <= 0) {
          exitCombatModeWin();
          return;
        }

        if (!enemyTurnAlreadyStarted) {
          moleTurn(); 
          enemyTurnActive = true;
          enemyTurnStartTime = millis();
          isMageTurn = false;
        }

        return;
      }
    }
  }
}



bool anyMoleAlive() {
  for (int i = 0; i < MOLE_COUNT; i++) {
    if (moleAliveArr[i]) return true;
  }
  return false;
}

int getMolesKilled() {
  int k = 0;
  for (int i = 0; i < MOLE_COUNT; i++) {
    if (!moleAliveArr[i]) k++;
  }
  return k;
}

void handleJoystickMoveCombat() {
  unsigned long now = millis();
  if (now - lastMoveTime < MOVE_INTERVAL) return;

  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);

  int newX = cursorX;
  int newY = cursorY;

  if (xVal < JOY_CENTER_MIN) newX = cursorX - 1;
  else if (xVal > JOY_CENTER_MAX) newX = cursorX + 1;

  if (yVal < JOY_CENTER_MIN) newY = cursorY + 1;
  else if (yVal > JOY_CENTER_MAX) newY = cursorY - 1;

  if (newX < 0) newX = 0;
  if (newX > 7) newX = 7;
  if (newY < 0) newY = 0;
  if (newY > 7) newY = 7;

  if (newX != cursorX || newY != cursorY) {
    cursorX = newX;
    cursorY = newY;
    lastMoveTime = now;
  }
}

void handleCombatButtons(bool drawPressed, bool clearPressed) {
  if (clearPressed) {
    clearCombatPattern();
    pixelsUsedThisTurn = 0;
  }

  if (drawPressed) {
    if (!combatPixels[cursorY][cursorX]) {
      if (pixelsUsedThisTurn < pixelsPerTurn) {
        combatPixels[cursorY][cursorX] = true;
        pixelsUsedThisTurn++;
      }
    }
  }
}

void updateCursorBlink() {
  unsigned long now = millis();
  if (now - lastCursorBlinkTime >= CURSOR_BLINK_INTERVAL) {
    lastCursorBlinkTime = now;
    cursorVisible = !cursorVisible;
  }
}

void drawCombatLayer() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (combatPixels[y][x]) {
        matrix.setLed(0, y, x, true);
      }
    }
  }
  if (cursorVisible) {
    matrix.setLed(0, cursorY, cursorX, true);
  }
}

void clearCombatPattern() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      combatPixels[y][x] = false;
    }
  }
}

void updateLCDGameCombat() {
  lcd.clear();

  // mage portrait
  lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.setCursor(1, 0);
  lcd.write(byte(1));
  lcd.setCursor(0, 1);
  lcd.write(byte(2));
  lcd.setCursor(1, 1);
  lcd.write(byte(3));

  // mage HP and shield
  lcd.setCursor(2, 0);
  lcd.print("HP");
  lcd.print(mageHP);

  // mole portrait
  lcd.setCursor(14, 0);
  lcd.write(byte(4));
  lcd.setCursor(15, 0);
  lcd.write(byte(5));
  lcd.setCursor(14, 1);
  lcd.write(byte(6));
  lcd.setCursor(15, 1);
  lcd.write(byte(7));

  lcd.setCursor(2, 1);
  lcd.print("SH");
  lcd.print(mageShield);

  // mole HP and shield
  lcd.setCursor(10, 0);
  lcd.print("HP");
  lcd.print(moleHP);

  lcd.setCursor(10, 1);
  lcd.print("SH");
  lcd.print(moleShield);

  // pixels left this turn
  int available = pixelsPerTurn - pixelsUsedThisTurn;
  if (available < 0) available = 0;
  lcd.setCursor(6, 0);
  lcd.print("px");
  lcd.print(available);
}


// SPELL CHECK

bool spellMatchesBitmap(const uint8_t pattern[8]) {
  for (int y = 0; y < 8; y++) {
    uint8_t row = 0;
    for (int x = 0; x < 8; x++) {
      row <<= 1;
      if (combatPixels[y][x]) row |= 0x01;
    }
    if (row != pattern[y]) return false;
  }
  return true;
}

bool isSpellAttack() {
  return spellMatchesBitmap(ATTACK_SPELL);
}

bool isSpellShield() {
  return (pixelsPerTurn >= 12) && spellMatchesBitmap(SHIELD_SPELL);
}

bool isSpellStun() {
  return spellMatchesBitmap(STUN_SPELL);
}
// COMBAT LOGIC + ANIMATIONS

enum AttackAnimState {
  ATTACK_ANIM_IDLE,
  ATTACK_ANIM_ON,
  ATTACK_ANIM_OFF,
  ATTACK_ANIM_DONE
};

AttackAnimState attackAnimState = ATTACK_ANIM_IDLE;
unsigned long attackAnimStart = 0;
int attackAnimStep = 0;

void startAttackAnimation() {
  attackAnimState = ATTACK_ANIM_ON;
  attackAnimStart = millis();
  attackAnimStep = 0;
}

void updateAttackAnimation() {
  unsigned long now = millis();
  if (attackAnimState == ATTACK_ANIM_IDLE || attackAnimState == ATTACK_ANIM_DONE) return;

  if (attackAnimState == ATTACK_ANIM_ON) {
    if (now - attackAnimStart >= 0) {
      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          matrix.setLed(0, y, x, true);
        }
      }
      attackAnimStart = now;
      attackAnimState = ATTACK_ANIM_OFF;
    }
    return;
  }

  if (attackAnimState == ATTACK_ANIM_OFF) {
    if (now - attackAnimStart >= ATTACK_ANIM_DELAY) {
      matrix.clearDisplay(0);
      attackAnimStep++;
      if (attackAnimStep >= 3) {
        attackAnimState = ATTACK_ANIM_DONE;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Attack!");
      } else {
        attackAnimStart = now;
        attackAnimState = ATTACK_ANIM_ON;
      }
    }
    return;
  }
}

void applyMageAttack() {
  int damage = 8;
  int effective = damage - moleShield;
  if (effective < 0) effective = 0;

  moleHP -= effective;
  if (moleHP < 0) moleHP = 0;

  moleShield = 0;
  startAttackAnimation();
}

enum ShieldAnimState {
  SHIELD_ANIM_IDLE,
  SHIELD_ANIM_ON,
  SHIELD_ANIM_OFF,
  SHIELD_ANIM_TEXT,
  SHIELD_ANIM_DONE
};

ShieldAnimState shieldAnimState = SHIELD_ANIM_IDLE;
unsigned long shieldAnimStart = 0;
int shieldAnimStep = 0;

void startShieldAnimation() {
  shieldAnimState = SHIELD_ANIM_ON;
  shieldAnimStart = millis();
  shieldAnimStep = 0;
}

void updateShieldAnimation() {
  unsigned long now = millis();
  if (shieldAnimState == SHIELD_ANIM_IDLE || shieldAnimState == SHIELD_ANIM_DONE) return;

  if (shieldAnimState == SHIELD_ANIM_ON) {
    // draw border
    matrix.clearDisplay(0);
    for (int i = 0; i < 8; i++) {
      matrix.setLed(0, 0, i, true);
      matrix.setLed(0, 7, i, true);
      matrix.setLed(0, i, 0, true);
      matrix.setLed(0, i, 7, true);
    }
    shieldAnimStart = now;
    shieldAnimState = SHIELD_ANIM_OFF;
    return;
  }

  if (shieldAnimState == SHIELD_ANIM_OFF) {
    if (now - shieldAnimStart >= SHIELD_ANIM_DELAY) {
      matrix.clearDisplay(0);
      shieldAnimStep++;
      if (shieldAnimStep >= 2) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Shield up");
        shieldAnimStart = now;
        shieldAnimState = SHIELD_ANIM_TEXT;
      } else {
        shieldAnimStart = now;
        shieldAnimState = SHIELD_ANIM_ON;
      }
    }
    return;
  }

  if (shieldAnimState == SHIELD_ANIM_TEXT) {
    if (now - shieldAnimStart >= SHIELD_ANIM_DELAY) {
      shieldAnimState = SHIELD_ANIM_DONE;
    }
    return;
  }
}

void shieldAnimation() {
  startShieldAnimation();
}

void applyMageShield() {
  mageShield += 4;
  shieldAnimation();
}

void moleTurn() {
  lastMoleAction = MOLE_ACT_NONE;

  // if stunned skip this turn
  if (moleStunTurns > 0) {
    moleStunTurns--;
    lastMoleAction = MOLE_ACT_NONE;  // mole is STUNNED
    return;
  }

  int r = random(0, 100);
  if (r < MOLE_ATTACK_CHANCE) {
    // mole ATTACK
    playAttackSound();
    int damage = 6;
    int effective = damage - mageShield;
    if (effective < 0) effective = 0;

    mageHP -= effective;
    if (mageHP < 0) mageHP = 0;

    mageShield = 0;
    lastMoleAction = MOLE_ACT_ATTACK;
  } else {
    // mole SHIELD
    playShieldSound();
    moleShield = 6;
    lastMoleAction = MOLE_ACT_SHIELD;
  }
}




void resetCombatTurn() {
  isMageTurn = true;
  pixelsUsedThisTurn = 0;
  clearCombatPattern();
}

// PAUSE

void loopPause() {
  int yVal = analogRead(JOY_Y_PIN);
  static unsigned long lastNavTime = 0;
  const unsigned long NAV_INTERVAL = 200;
  unsigned long now = millis();

  if (now - lastNavTime > NAV_INTERVAL) {
    if (yVal > JOY_CENTER_MAX) {
      pauseMenuIndex--;
      lastNavTime = now;
    } else if (yVal < JOY_CENTER_MIN) {
      pauseMenuIndex++;
      lastNavTime = now;
    }
    if (pauseMenuIndex < 0) pauseMenuIndex = 2;
    if (pauseMenuIndex > 2) pauseMenuIndex = 0;

  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Game is paused");
  lcd.setCursor(0, 1);
  lcd.print("> ");
  switch (pauseMenuIndex) {
    case 0: lcd.print("Resume     "); break;
    case 1: lcd.print("Show Spells"); break;
    case 2: lcd.print("Exit       "); break;
  }
    if (isJoystickButtonPressedEdge()) {
      switch (pauseMenuIndex) {
    case 0: // Resume
      gameState = savedGameState;
      break;

    case 1: // Show Spells
      gameState = STATE_PAUSE_SPELLS;
      break;

    case 2: // Exit
      gameState = STATE_MENU;
      menuState = MENU_MAIN;
      lcd.clear();
      break;
  }
}
}

void loopPauseSpells() {
  int yVal = analogRead(JOY_Y_PIN);
  static unsigned long lastNavTime = 0;
  const unsigned long NAV_INTERVAL = 200;
  unsigned long now = millis();

  if (now - lastNavTime > NAV_INTERVAL) {
    if (yVal > JOY_CENTER_MAX) {
      if (spellsPage > 0) spellsPage--;
      lastNavTime = now;
    } else if (yVal < JOY_CENTER_MIN) {
      if (spellsPage < SPELLS_PAGES - 1) spellsPage++;
      lastNavTime = now;
    }
  }

  renderSpellsPage();

  if (isJoystickButtonPressedEdge()) {
    clearMatrix();
    gameState = STATE_PAUSE;
    lcd.clear();
  }
}

// WIN / LOSE

void loopWin() {
  clearMatrix();
  if (isJoystickButtonPressedEdge()) {
    gameState = STATE_MENU;
    menuState = MENU_MAIN;
    lcd.clear();
  }
}

void loopLose() {
  clearMatrix();
  if (isJoystickButtonPressedEdge()) {
    gameState = STATE_MENU;
    menuState = MENU_MAIN;
    lcd.clear();
  }
}

// helpers

bool isJoystickButtonPressedEdge() {
  bool current = (digitalRead(JOY_BTN_PIN) == LOW);
  bool pressed = (!lastJoyBtnState && current);
  lastJoyBtnState = current;
  return pressed;
}

bool isPauseButtonPressedEdge() {
  bool current = (digitalRead(PAUSE_BTN_PIN) == LOW);
  bool pressed = (!lastPauseBtnState && current);
  lastPauseBtnState = current;
  return pressed;
}

long readDistanceCm() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 9999; // no echo

  long distance = duration * 0.034 / 2; // cm
  return distance;
}