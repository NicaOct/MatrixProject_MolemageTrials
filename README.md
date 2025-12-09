# MatrixProject_MolemageTrials
# Matrix Project Checkpoint 1

This project is a dungeon-crawler prototype for Arduino Uno that uses an 8×8 LED matrix as the main playfield and a 16×2 LCD as a HUD. The player explores a small dungeon, finds a mole enemy and defeats it by drawing a spell pattern directly on the LED matrix.

***

## Objective

The goal is to implement a small but complete game loop on constrained hardware:

- Explore a dungeon composed of 8×8 rooms.
- Detect proximity to an enemy (the mole) and transition into a separate combat mode.
- In combat, let the player draw a spell with a cursor and buttons, then validate the drawn pattern.
- Provide a persistent HUD on the LCD with a custom “portrait” and live status info.

The code is structured for readability and for use as a base for later checkpoints or feature upgrades.

***

## Hardware Overview

Required components:

- Arduino Uno
- 8×8 LED matrix driven by MAX7219 (via LedControl)
- 16×2 LCD (LiquidCrystal)
- Joystick module
  - X axis → A0
  - Y axis → A1
  - Button → A2
- 2× push buttons for combat
  - Draw → A3 (to GND, INPUT_PULLUP)
  - Clear → A4 (to GND, INPUT_PULLUP)
- Breadboard and jumper wires

Pin mapping and usage are explicit in the sketch so the hardware wiring is straightforward to reproduce.

***

## Game States and Flow

The game uses a small finite state machine to keep logic clean:

- `STATE_MENU` – Title and start prompt.
- `STATE_EXPLORE` – Explore dungeon rooms on the matrix.
- `STATE_COMBAT` – Draw and submit a spell pattern.
- `STATE_WIN` – Display victory after killing the mole.

`loop()` simply dispatches to `loopMenu()`, `loopExplore()`, `loopCombat()` or `loopWin()` depending on the current state. Each state is responsible for:

- Reading relevant input
- Updating internal state
- Rendering matrix and LCD

This keeps menu, exploration, and combat code clearly separated.

***

## Rooms, Player and Mole

### Room Representation

Rooms are stored as a 3D boolean array:

- `rooms[NUM_ROOMS][8][8]`, where `true` = wall, `false` = empty space.

Two rooms are defined:

- Room 0: starting area with outer walls, some internal walls, and a bottom door.
- Room 1: the mole’s room with its own wall layout and a top door back to Room 0.

`drawCurrentRoom()` iterates over the 8×8 grid and lights all wall cells on the matrix. `isWall(x, y)` encapsulates bounds checking plus room lookup, providing a single collision function for movement and doors.

### Player Movement

The player is a blinking LED on the matrix:

- Position: `playerX`, `playerY`.
- Movement: joystick axes read via `analogRead()` on A0 and A1.
- Thresholds (`JOY_CENTER_MIN`, `JOY_CENTER_MAX`) map analog values to left/right/up/down.
- Movement is rate-limited by `MOVE_INTERVAL` using `millis()` so holding the joystick does not move the player too fast.

Collision is handled by checking candidate moves with `isWall()`. If the next tile is not a wall, the player position is updated.

The previous position is always stored in `prevPlayerX` and `prevPlayerY` before movement. This is important for safely returning the player after combat (see below).

### Mole Enemy

The mole is a simple stationary enemy:

- Fixed coordinates (`moleX`, `moleY`) in Room 1.
- Uses a blink timer, similar to the player, to toggle visibility.
- Controlled by a `moleAlive` flag so it disappears and stops triggering combat after being killed.

Combat is triggered if the player is within a 3×3 square around the mole, excluding the mole’s exact tile. This check is handled in `isNearMole()`.

***

## Doors and Room Transitions

Door tiles connect the two rooms:

- Room 0 has a door at `(door0_x, door0_y)` on the bottom wall.
- Room 1 has a door at `(door1_x, door1_y)` on the top wall.

`checkDoorTransition()` monitors the player’s coordinates. If the player steps onto a door tile:

- `currentRoom` is switched.
- The player is placed just inside the opposite room, one tile away from the door, to prevent immediate back-and-forth thrashing.

Rooms are fully static, but the approach (boolean grid + door positions) is ready for more rooms or procedural layouts later.

***

## Combat Mode and Spell Drawing

When the player gets close enough to the mole and `moleAlive` is still true, `enterCombatMode()` switches the state to `STATE_COMBAT` and sets up for drawing:

- Clears the matrix.
- Resets the `combatPixels[8][8]` buffer.
- Moves the cursor to the center (3,3).

### Cursor and Drawing

In combat, the joystick controls a cursor instead of the player:

- Position: `cursorX`, `cursorY`.
- Same axis thresholds and movement interval logic as in exploration, but clamped strictly to 0–7.

Drawing is handled via a dedicated buffer:

- `combatPixels[8][8]` is a boolean array where `true` = drawn pixel.
- When the draw button (A3) is held down, the current cursor cell is set to `true`.
- `drawCombatLayer()` draws all `true` cells first, then overlays the blinking cursor.

The clear button (A4) calls `clearCombatPattern()`, which sets the entire buffer back to `false`.

### Spell Submission

The joystick button (A2) is used to submit the spell:

- A simple edge detection function (`isJoystickButtonPressedEdge()`) ensures the spell is evaluated only once per press.
- On submit, `spellMatchesPattern()` checks the content of `combatPixels`.

The current spell is a strict 2×2 block in the center:

- Required ON cells: (3,3), (4,3), (3,4), (4,4).
- All other cells must be OFF.

If the pattern matches:

- `exitCombatMode(true)` sets `moleAlive = false` and transitions to `STATE_WIN`.
- The LCD displays a “Mole defeated / You win” message.

If the pattern does not match:

- `exitCombatMode(false)` transitions back to `STATE_EXPLORE`.
- The player’s position is restored from `prevPlayerX`, `prevPlayerY`, so they return just outside the mole’s trigger radius, instead of immediately re-entering combat.

This data flow makes it easy to change or add new spells later by changing only the validation function.

***

## HUD and LCD Usage

The LCD is used as a permanent HUD during exploration and combat, with only menu/win screens overriding it temporarily.

### Custom Glyphs

`setupLCD()` defines four custom characters (indices 0–3) that form a 2×2 “portrait”:

- A full block
- An upper half block
- A lower half block
- A hollow box

`drawHudStatic()`:

- Clears the LCD.
- Draws the four glyphs as a 2×2 block at (0,0), (1,0), (0,1), (1,1).
- Prints the mission text `Find the Mole` starting at column 4 on the second row.

This gives a simple visual identity to the player and leaves room for status text.

### Dynamic HUD Content

The HUD is updated differently per state:

- In **explore**, `updateLCDGameExplore()` prints:
  - On row 0, starting at column 4: `playerX,playerY` (with minimal spacing to keep the layout stable).
  - Row 1 keeps the mission text.
- In **combat**, `updateLCDGameCombat()` prints:
  - On row 0, starting at column 4: `cursorX,cursorY`.
  - On row 1: reprints `Find the Mole`.

On entering explore or combat from other states, `drawHudStatic()` is called again to restore the base HUD before coordinates are written.

***

## Showcase
<img width="1608" height="1068" alt="image" src="https://github.com/user-attachments/assets/10e53068-6e63-418a-9b8a-e8aba682a46d" />
![ ](https://github.com/user-attachments/assets/693b7fa8-1403-4b59-b4bb-e7304f273424)


[Play it yourself!](https://wokwi.com/projects/449226081809266689)
[Video](https://youtu.be/bY-bsI6A_cs)

***


# MatrixProject_MolemageTrials
# Matrix Project Checkpoint 2

This checkpoint extends the original Molemage Trials prototype from Checkpoint 1. The game still runs on Arduino Uno with an 8×8 LED matrix as the main playfield and a 16×2 LCD as a HUD, but now uses a 16×16 dungeon map, multiple enemies, potions, a richer combat system and a pause menu.

***

## Objective

Compared to Checkpoint 1, this version aims to:

- Replace the two fixed 8×8 rooms with a 16×16 tile map and an 8×8 scrolling viewport.
- Support multiple mole enemies and track how many have been killed.
- Add healing potions that are visible on the matrix and restore mage HP.
- Expand combat with two spells (attack and shield) and a pixel budget per turn.
- Add a dedicated pause button and a pause menu (Resume / Show Spells / Exit).
- Add an optional intro story and a first‑spell tutorial sequence.
- Change the HUD to always show mage HP and mole kills instead of coordinates.

***

## Hardware Overview

Components:

- Arduino Uno  
- 8×8 LED matrix with MAX7219 (LedControl)  
- 16×2 LCD (LiquidCrystal)  
- Joystick  
  - X axis → A0  
  - Y axis → A1  
  - Switch → A2  
- Combat buttons  
  - Draw  → A3 (INPUT_PULLUP)  
  - Clear → A4 (INPUT_PULLUP)  
- Pause button  
  - Pause → A5 (INPUT_PULLUP)

All pin assignments are defined as constants in the sketch, so wiring can be read directly from the code.

***

## Game States and Flow

The game now uses these states:

- `STATE_INTRO` – Optional story text (used if `SKIP_INTRO == false`).
- `STATE_TUTORIAL_INTRO` – First‑spell tutorial (learn the attack symbol).
- `STATE_MENU` – Main menu: Start, Tutorial, Spells, Credits.
- `STATE_EXPLORE` – Explore the 16×16 dungeon via an 8×8 viewport.
- `STATE_COMBAT` – Spell‑based combat with a per‑turn pixel budget.
- `STATE_PAUSE` – Pause menu (Resume, Show Spells, Exit).
- `STATE_PAUSE_SPELLS` – Spell viewer from pause.
- `STATE_WIN` – Final victory after all moles are defeated.
- `STATE_LOSE` – Game over when mage HP reaches 0.

The main `loop()` switches on `gameState` and calls one of:

- `loopIntro()`
- `loopTutorialIntro()`
- `loopMenu()`
- `loopExplore()`
- `loopCombat()`
- `loopPause()`
- `loopPauseSpells()`
- `loopWin()`
- `loopLose()`

The pause button (A5) is checked in `loopExplore()` and `loopCombat()`. When pressed:

- `savedGameState` stores the previous state.
- `gameState` is set to `STATE_PAUSE`.

This allows the game to resume cleanly back into explore or combat after pause.

***

## Map, Player, Moles and Potions

### 16×16 Map and 8×8 Viewport

The dungeon is a 16×16 grid:

- `rawMap[16][16]` – 0 = floor, 1 = wall.  
- `mapData[16][16]` – boolean version used by `isWallGlobal()`.

Because the LED matrix is only 8×8, the code uses an 8×8 viewport:

- `viewX`, `viewY` = top‑left tile of what is currently visible.  
- `updateViewport()` centers the viewport around the player and clamps it so the window never goes outside the map.  
- `drawViewport()` lights all wall tiles inside the viewport on the matrix.

### Player

The player is stored in global coordinates:

- `playerGX`, `playerGY` – current tile.  
- Movement uses joystick analog values with thresholds to detect directions and is limited by `MOVE_INTERVAL` so movement is not too fast.  
- `isWallGlobal(gx, gy)` prevents walking through walls or outside the map.  
- The player LED still blinks using `playerVisible` and `BLINK_INTERVAL`.

### Multiple Moles

Instead of a single mole, there are several:

- `moleGX[i]`, `moleGY[i]` – positions on the 16×16 map.  
- `moleAliveArr[i]` – per‑mole alive flags.  
- `moleVisible` – global blink flag updated with `MOLE_BLINK_INTERVAL`.

In `initMap()`, the map cells under the moles are forced to floor so the player can walk next to them.

`isNearMole()` finds an alive mole near the player (in a 3×3 area) and stores its index in `currentMoleIndex`. `anyMoleAlive()` and `getMolesKilled()` are used to check win condition and HUD display.

### Potions

Potions are represented similarly:

- `potionGX[i]`, `potionGY[i]` – positions.  
- `potionActive[i]` – whether each potion is still available.  
- `potionVisible` – blink flag with its own interval.

`checkPotionPickup()`:

- Detects when the player steps on an active potion.  
- Heals the mage by 2 HP (up to a max of 5).  
- Shows a short “heal potion picked & used” message using a potion glyph.  
- Calls `drawHudStatic()` afterwards to restore the mage portrait HUD.

***

## Combat System and Spells

Combat still uses the 8×8 matrix, but is more advanced than in Checkpoint 1.

Key data:

- Cursor position: `cursorX`, `cursorY` with blinking (`cursorVisible`).  
- Drawing buffer: `combatPixels[8][8]`.  
- HP: `mageHP`, `moleHP`.  
- Shields: `mageShield`, `moleShield`.  
- Pixel budget:
  - `basePixelsPerTurn = 8`.
  - `pixelsPerTurn` – how many pixels can be drawn per turn.
  - `pixelsUsedThisTurn` – pixels already drawn.

When entering combat (`enterCombatMode()`):

- The buffer is cleared.  
- HP and shields are reset.  
- `pixelsPerTurn` is computed from a base plus a bonus if at least one mole was previously killed.  
- `pixelsUsedThisTurn` is reset to 0.

On the mage’s turn:

- The joystick moves the cursor.  
- Draw button (A3) sets the current cell to true if the pixel budget allows.  
- Clear button (A4) wipes the pattern and resets `pixelsUsedThisTurn`.  
- `drawCombatLayer()` shows drawn pixels plus a blinking cursor.  
- `updateLCDGameCombat()` displays HP, shields and remaining pixels (`px<remaining>`).

### Spells

Spells are stored as 8‑row bitmaps:

- `ATTACK_SPELL[8]`.  
- `SHIELD_SPELL[8]`.

`spellMatchesBitmap()` converts each row of `combatPixels` into an 8‑bit value and compares it with the pattern. Two helper functions use this:

- `isSpellAttack()` – matches `ATTACK_SPELL`.  
- `isSpellShield()` – matches `SHIELD_SPELL` and requires `pixelsPerTurn >= 12`.

When the joystick switch (A2) is pressed in combat:

- If attack spell:
  - `applyMageAttack()` deals 1 damage reduced by `moleShield` (minimum 0), updates `moleHP`, clears `moleShield` and plays an attack animation.  
- If shield spell:
  - `applyMageShield()` sets `mageShield = 1` and plays a shield animation.  
- Then `moleTurn()` runs:
  - With 70% chance the mole attacks (damage reduced by `mageShield`), otherwise the mole gains `moleShield = 1`.  
- If HP reaches 0 on either side, the game switches to `STATE_WIN` or `STATE_LOSE`.  
- Otherwise `resetCombatTurn()` clears the pattern and resets the pixel counter for the next spell.

### Killing a Mole and Progression

`exitCombatModeWin()`:

- Marks the current mole as dead (`moleAliveArr[currentMoleIndex] = false`).  
- Sets `firstMoleKilled = true`.  
- Increases `pixelsPerTurn` by 4 so future combats allow more drawing.  
- If all moles are dead, switches to `STATE_WIN` and shows a final “All moles defeated” message.  
- Otherwise returns to `STATE_EXPLORE`, clears combat, redraws HUD and updates the viewport.

`exitCombatModeLose()` switches to `STATE_LOSE` and shows a death message.

***

## HUD and Menus (LCD)

### Mage HUD

Custom characters 0–3 form a 2×2 mage portrait at:

- (0,0), (1,0), (0,1), (1,1).

`drawHudStatic()`:

- Clears the LCD.  
- Draws the four glyphs in the 2×2 block.  
- Leaves the rest of the line for dynamic text.

### Explore HUD

During exploration, `updateLCDGameExplore()` prints:

- On row 0, from column 2: `HP:<mageHP>`.  
- On row 1, from column 2: `Mole:<kills>/<MOLE_COUNT>`.

The mage portrait remains always visible in the left 2×2 corner.

### Combat HUD

`updateLCDGameCombat()`:

- Redraws the mage portrait.  
- Shows mage HP and shield status.  
- Shows mole HP and whether it has shield or is ready to attack.  
- Shows remaining pixels for the current turn (`px<available>`).

### Intro and Tutorial

If `SKIP_INTRO` is `false`:

- Start from the main menu leads to `STATE_INTRO` (short story text).  
- Then `STATE_TUTORIAL_INTRO`, which:  
  - Shows basic instructions.  
  - Blinks the attack pattern on the matrix.  
  - Asks the player to draw and submit a matching spell.  
  - On success, jumps directly into `STATE_EXPLORE`.

If `SKIP_INTRO` is `true`, Start goes straight to exploration.

### Pause Menu and Spell Viewer

In `STATE_EXPLORE` and `STATE_COMBAT`, pressing Pause (A5):

- Saves the current state to `savedGameState`.  
- Switches to `STATE_PAUSE`.

`loopPause()`:

- Shows “Game is paused” on row 0.  
- Shows a selectable option on row 1:
  - Resume  
  - Show Spells  
  - Exit  
- Joystick Y changes `pauseMenuIndex`.  
- Joystick switch selects:
  - Resume → `gameState = savedGameState`.  
  - Show Spells → `gameState = STATE_PAUSE_SPELLS`.  
  - Exit → back to `STATE_MENU` / `MENU_MAIN`.

`loopPauseSpells()`:

- Reuses `renderSpellsPage()` and `spellsPage` from the main menu.  
- Shows both spells (Attack and Shield) with names on the LCD and bitmaps on the matrix.  
- Joystick switch returns to `STATE_PAUSE`.

***

## Main Differences vs Checkpoint 1

- World: from 2 fixed 8×8 rooms to a 16×16 map with an 8×8 scrolling viewport.  
- Enemies: from one mole to multiple moles with kill count and HP/shield combat.  
- Items: new healing potions visible on the matrix and applied on pickup.  
- Combat: from one strict 2×2 spell to two bitmap spells plus a pixel budget that grows (+4 per kill).  
- UI: new pause button and pause menu, a spells viewer inside pause, and a HUD focused on HP and kills.  
- Narrative: optional intro story and a first‑spell tutorial integrated into the game flow.

