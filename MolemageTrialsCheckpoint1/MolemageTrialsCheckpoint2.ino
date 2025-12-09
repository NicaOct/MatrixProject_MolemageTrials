#include <LedControl.h> 
#include <LiquidCrystal.h>

//  ---- Pins  ----
const int MATRIX_DIN  = 12;
const int MATRIX_CLK  = 11;
const int MATRIX_LOAD = 10;

const int JOY_X_PIN   = A0;
const int JOY_Y_PIN   = A1;

const int JOY_BTN_PIN = A2;  // joystick switch

const int DRAW_BTN_PIN  = A3; // draw when pressed
const int CLEAR_BTN_PIN = A4; // clear drawing when pressed

const int LCD_RS = 9;
const int LCD_EN = 8;
const int LCD_D4 = 7;
const int LCD_D5 = 6;
const int LCD_D6 = 5;
const int LCD_D7 = 4;

//  ---- Objects  ----
LedControl matrix = LedControl(MATRIX_DIN, MATRIX_CLK, MATRIX_LOAD, 1);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

//  ---- Game state  ----
enum GameState {
  STATE_MENU,
  STATE_EXPLORE,
  STATE_COMBAT,
  STATE_WIN
};

GameState gameState = STATE_MENU;

//  ---- Rooms  ----
const int NUM_ROOMS = 2;
bool rooms[NUM_ROOMS][8][8];   // true = wall

// door positions
int door0_x = 3, door0_y = 7;  // room 0 -> room 1 bottom
int door1_x = 3, door1_y = 0;  // room 1 -> room 0 top

int currentRoom = 0;

//  ---- Player data (explore)  ----
int playerX = 1;
int playerY = 1;
int prevPlayerX = 1;
int prevPlayerY = 1;
bool playerVisible = true;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 300;

//  ---- Enemy (mole) in room 1  ----
int moleX = 4;
int moleY = 4;
bool moleVisible = true;
unsigned long lastMoleBlinkTime = 0;
const unsigned long MOLE_BLINK_INTERVAL = 400;
bool moleAlive = true;

//  ---- Joystick  ----
const int JOY_CENTER_MIN = 400;
const int JOY_CENTER_MAX = 624;

unsigned long lastMoveTime = 0;
const unsigned long MOVE_INTERVAL = 150;

//  ---- Combat mode  ----
int cursorX = 3;
int cursorY = 3;
bool cursorVisible = true;
unsigned long lastCursorBlinkTime = 0;
const unsigned long CURSOR_BLINK_INTERVAL = 250;

bool combatPixels[8][8];  // drawn spell pattern

// switch edge detect
bool lastJoyBtnState = HIGH;

//  ---- Other game state  ----
long score = 0;

//  ---- Function declarations  ----
void setupMatrix();
void setupLCD();
void setupJoystick();
void initRooms();

void drawHudStatic();

void loopMenu();
void loopExplore();
void loopCombat();
void loopWin();

void drawCurrentRoom();
void clearMatrix();
void drawPlayerAndMole();
void updatePlayerBlink();
void updateMoleBlink();
void handleJoystickMoveExplore();
bool isWall(int x, int y);
void updateLCDGameExplore();
bool isNearMole();
void checkDoorTransition();

void handleJoystickMoveCombat();
void drawCombatLayer();
void updateCursorBlink();
void handleCombatButtons(bool drawPressed, bool clearPressed);
void clearCombatPattern();
void enterCombatMode();
void exitCombatMode(bool killedMole);
void updateLCDGameCombat();

bool isJoystickButtonPressedEdge();
bool spellMatchesPattern();

//  ---- Setup  ----
void setup() {
  setupMatrix();
  setupLCD();
  setupJoystick();
  initRooms();

  playerX = 1;
  playerY = 1;
  prevPlayerX = 1;
  prevPlayerY = 1;
  score = 0;
  moleAlive = true;

  gameState = STATE_MENU;
}

//  ---- Loop  ----
void loop() {
  switch (gameState) {
    case STATE_MENU:
      loopMenu();
      break;
    case STATE_EXPLORE:
      loopExplore();
      break;
    case STATE_COMBAT:
      loopCombat();
      break;
    case STATE_WIN:
      loopWin();
      break;
  }
}

//  ---- Setup helpers  ----
void setupMatrix() {
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 8);
  matrix.clearDisplay(0);
}

void setupLCD() {
  lcd.begin(16, 2);
  lcd.clear();

  // glyph 0 - top-left
  byte glyph0[8] = {
    B00000, B00001, B00011, B00111, B01111, B00110, B00100, B00011
  };

  // glyph 1 - top-right
  byte glyph1[8] = {
    B11000, B10000, B10000, B11100, B11110, B01100, B00100, B11001
  };

  // glyph 2 - lower-left
  byte glyph2[8] = {
    B00001, B00011, B00111, B00101, B00101, B00011, B00111, B01111
  };

  // glyph 3 - lower-right
  byte glyph3[8] = {
    B10011, B10010, B11110, B10011, B10001, B11011, B11010, B11010
  };

  lcd.createChar(0, glyph0);
  lcd.createChar(1, glyph1);
  lcd.createChar(2, glyph2);
  lcd.createChar(3, glyph3);

  drawHudStatic();
}

void setupJoystick() {
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);
  pinMode(DRAW_BTN_PIN, INPUT_PULLUP);
  pinMode(CLEAR_BTN_PIN, INPUT_PULLUP);
}

// draw static HUD art and objective text
void drawHudStatic() {
  lcd.clear();

  // 2x2 HUD block for mage formed from 4 glyphs
  lcd.setCursor(0, 0);
  lcd.write(byte(0)); // top-left
  lcd.setCursor(1, 0);
  lcd.write(byte(1)); // top-right
  lcd.setCursor(0, 1);
  lcd.write(byte(2)); // bottom-left
  lcd.setCursor(1, 1);
  lcd.write(byte(3)); // bottom-right

  // objective text
  lcd.setCursor(3, 1);
  lcd.print("Find the Mole");
}

//  ---- Rooms layout  ----
void initRooms() {
  for (int r = 0; r < NUM_ROOMS; r++) {
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        rooms[r][y][x] = false;
      }
    }
  }

  // Room 0 layout
  for (int x = 0; x < 8; x++) {
    rooms[0][0][x] = true;
    rooms[0][7][x] = true;
  }
  for (int y = 0; y < 8; y++) {
    rooms[0][y][0] = true;
    rooms[0][y][7] = true;
  }
  for (int x = 2; x <= 5; x++) {
    rooms[0][2][x] = true;
    rooms[0][3][x] = true;
  }
  rooms[0][5][2] = true;
  rooms[0][6][2] = true;
  rooms[0][door0_y][door0_x] = false;

  // Room 1 layout
  for (int x = 0; x < 8; x++) {
    rooms[1][0][x] = true;
    rooms[1][7][x] = true;
  }
  for (int y = 0; y < 8; y++) {
    rooms[1][y][0] = true;
    rooms[1][y][7] = true;
  }
  for (int y = 1; y <= 6; y++) {
    rooms[1][y][5] = true;
    rooms[1][y][6] = true;
  }
  rooms[1][2][2] = true;
  rooms[1][3][2] = true;
  rooms[1][3][3] = true;
  rooms[1][door1_y][door1_x] = false;
  rooms[1][moleY][moleX] = false;
}

//  ---- MENU state  ----
void loopMenu() {
  lcd.setCursor(0, 0);
  lcd.print("Molemage Trials");
  lcd.setCursor(0, 1);
  lcd.print("> START         ");

  if (isJoystickButtonPressedEdge()) {
    drawHudStatic();
    gameState = STATE_EXPLORE;
    currentRoom = 0;
    playerX = 1;
    playerY = 1;
    prevPlayerX = 1;
    prevPlayerY = 1;
    score = 0;
    moleAlive = true;
    moleVisible = true;
    lastBlinkTime = millis();
    lastMoleBlinkTime = millis();
  }
}

//  ---- EXPLORE state  ----
void loopExplore() {
  handleJoystickMoveExplore();
  checkDoorTransition();
  updatePlayerBlink();
  updateMoleBlink();

  clearMatrix();
  drawCurrentRoom();
  drawPlayerAndMole();

  if (currentRoom == 1 && moleAlive && isNearMole()) {
    enterCombatMode();
    return;
  }

  updateLCDGameExplore();
}

void drawCurrentRoom() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (rooms[currentRoom][y][x]) {
        matrix.setLed(0, y, x, true);
      }
    }
  }
}

void clearMatrix() {
  matrix.clearDisplay(0);
}

void drawPlayerAndMole() {
  if (playerVisible) {
    matrix.setLed(0, playerY, playerX, true);
  }
  if (currentRoom == 1 && moleAlive && moleVisible) {
    matrix.setLed(0, moleY, moleX, true);
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
  if (!moleAlive) return;
  if (now - lastMoleBlinkTime >= MOLE_BLINK_INTERVAL) {
    lastMoleBlinkTime = now;
    moleVisible = !moleVisible;
  }
}

bool isNearMole() {
  int dx = playerX - moleX;
  int dy = playerY - moleY;
  if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
    if (!(dx == 0 && dy == 0)) return true;
  }
  return false;
}

void checkDoorTransition() {
  if (currentRoom == 0 && playerX == door0_x && playerY == door0_y) {
    currentRoom = 1;
    playerX = door1_x;
    playerY = door1_y + 1;
  } else if (currentRoom == 1 && playerX == door1_x && playerY == door1_y) {
    currentRoom = 0;
    playerX = door0_x;
    playerY = door0_y - 1;
  }
}

void handleJoystickMoveExplore() {
  unsigned long now = millis();
  if (now - lastMoveTime < MOVE_INTERVAL) return;

  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);

  int newX = playerX;
  int newY = playerY;

  if (xVal < JOY_CENTER_MIN) newX = playerX - 1;
  else if (xVal > JOY_CENTER_MAX) newX = playerX + 1;

  if (yVal < JOY_CENTER_MIN) newY = playerY + 1;
  else if (yVal > JOY_CENTER_MAX) newY = playerY - 1;

  bool moved = false;

  if (!isWall(newX, playerY) && newX != playerX) {
    prevPlayerX = playerX;
    prevPlayerY = playerY;
    playerX = newX;
    moved = true;
  }
  if (!isWall(playerX, newY) && newY != playerY) {
    prevPlayerX = playerX;
    prevPlayerY = playerY;
    playerY = newY;
    moved = true;
  }

  if (moved) {
    lastMoveTime = now;
    score++;
  }
}

bool isWall(int x, int y) {
  if (x < 0 || x > 7 || y < 0 || y > 7) return true;
  return rooms[currentRoom][y][x];
}

void updateLCDGameExplore() {
  // coords after HUD 2x2 block
  lcd.setCursor(4, 0);
  if (playerX < 10) lcd.print(" ");
  lcd.print(playerX);
  lcd.print(",");
  if (playerY < 10) lcd.print(" ");
  lcd.print(playerY);
  lcd.print("   ");
}

//  ---- COMBAT state  ----
void enterCombatMode() {
  gameState = STATE_COMBAT;
  clearCombatPattern();
  cursorX = 3;
  cursorY = 3;
  cursorVisible = true;
  lastCursorBlinkTime = millis();

  drawHudStatic();
}

void exitCombatMode(bool killedMole) {
  if (killedMole) {
    moleAlive = false;
    gameState = STATE_WIN;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mole defeated");
    lcd.setCursor(0, 1);
    lcd.print("You win");
  } else {
    gameState = STATE_EXPLORE;
    playerX = prevPlayerX;
    playerY = prevPlayerY;
    drawHudStatic();
  }
}

void loopCombat() {
  handleJoystickMoveCombat();

  bool drawPressed  = (digitalRead(DRAW_BTN_PIN)  == LOW);
  bool clearPressed = (digitalRead(CLEAR_BTN_PIN) == LOW);
  handleCombatButtons(drawPressed, clearPressed);
  updateCursorBlink();

  clearMatrix();
  drawCombatLayer();

  updateLCDGameCombat();

  if (isJoystickButtonPressedEdge()) {
    bool ok = spellMatchesPattern();
    exitCombatMode(ok);
  }
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
  if (drawPressed) {
    combatPixels[cursorY][cursorX] = true;
  }
  if (clearPressed) {
    clearCombatPattern();
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
  // show cursor coords in HUD slot
  lcd.setCursor(3, 0);
  lcd.print("Cast a Spell");

  lcd.setCursor(3, 1);
  lcd.print("Kill the Mole");
}

//  ---- Spell pattern check  ----
// Spell = small square in center:
// cells (3,3) (4,3) (3,4) (4,4) must be on
// all other cells must be off
bool spellMatchesPattern() {
  int sx = 3, sy = 3;
  for (int y = sy; y <= sy + 1; y++) {
    for (int x = sx; x <= sx + 1; x++) {
      if (!combatPixels[y][x]) return false;
    }
  }
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (y >= sy && y <= sy + 1 && x >= sx && x <= sx + 1) continue;
      if (combatPixels[y][x]) return false;
    }
  }
  return true;
}

//  ---- WIN state  ----
void loopWin() {
  clearMatrix();
  // stay on win screen
}

//  ---- Button helpers  ----
bool isJoystickButtonPressedEdge() {
  bool current = digitalRead(JOY_BTN_PIN) == LOW;
  bool pressed = (!lastJoyBtnState && current);
  lastJoyBtnState = current;
  return pressed;
}