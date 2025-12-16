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
## Showcase
![2 1](https://github.com/user-attachments/assets/1514b8b1-fb73-40e1-b06c-e404cc1a42e5)
![2 2](https://github.com/user-attachments/assets/f4f1fca7-9a7e-4e65-a5c9-76371b568895)
![2 3](https://github.com/user-attachments/assets/d033a8f1-e9a4-4f14-8cbe-fd0d644d501a)



[Play it yourself!](https://wokwi.com/projects/449886919352617985)
[Video](https://youtu.be/iJmGpDPZHa4)

***



# Molemage Trials

**Molemage Trials** is a turn–based dungeon crawler built on Arduino UNO, combining an 8×8 LED matrix, a 16×2 LCD, a joystick, buttons, a buzzer, and an HC‑SR04 distance sensor into a cohesive, fully playable game.

The project explores:
- non‑blocking game loops with `millis()` instead of `delay()`,  
- a simple but complete game architecture (states, combat system, spells, map & camera),  
- embedding game‑design ideas (risk–reward, resource management, stun/escape mechanics) into very limited hardware.

---

## Hardware Requirements

- **Arduino UNO** (or compatible)
- **8×8 LED matrix** with **MAX7219** driver  
- **16×2 LCD** using `LiquidCrystal`
- **2‑axis joystick** with push button
- **3 buttons**:
  - DRAW (spell drawing)
  - CLEAR (erase spell)
  - PAUSE
- **Piezo buzzer** (connected to `BUZZER_PIN`)
- **HC‑SR04 ultrasonic distance sensor** (escape mechanic)
- Wires, breadboard / PCB as needed

### Pinout (example)

- LED Matrix (MAX7219):
  - `DIN` → 12  
  - `CLK` → 11  
  - `LOAD` / `CS` → 10

- LCD:
  - `RS` → 9  
  - `EN` → 8  
  - `D4` → 7  
  - `D5` → 6  
  - `D6` → 5  
  - `D7` → 4

- Joystick:
  - `X` → A0  
  - `Y` → A1  
  - `Button` → A2 (INPUT_PULLUP)

- Buttons:
  - DRAW → A3 (INPUT_PULLUP)  
  - CLEAR → A4 (INPUT_PULLUP)  
  - PAUSE → A5 (INPUT_PULLUP)

- Buzzer:
  - `BUZZER_PIN` → 13

- HC‑SR04:
  - `TRIG` → 2  
  - `ECHO` → 3

Pin numbers in code can be adjusted, but README assumes the mapping above.

---

## How to Build & Run

1. **Wire the hardware** according to the pinout above.
2. Open the `.ino` sketch in **Arduino IDE**.
3. Make sure you have:
   - `LedControl.h`
   - `LiquidCrystal.h`
4. Upload the sketch to the Arduino UNO.
5. After reset, the game starts in the **Main Menu**.

---

## Controls

### Global

- **Joystick button**:
  - In menus: confirm / select.
  - In explore: interact (enter combat when near mole is automatic).
- **Pause button**:
  - In explore/combat: open pause menu.

### Explore Mode

- Move with joystick (4 directions).  
- Walk around the map, collect **heal potions**, and approach **moles** to start combat.

### Combat Mode

- **Joystick**: move the cursor on the 8×8 matrix.
- **DRAW button**: paint pixels at cursor (up to a per‑turn limit).
- **CLEAR button**: erase current drawing for this turn.
- **Joystick button**: cast the spell represented by the drawn pattern:
  - **Basic Attack**: defined by `ATTACK_SPELL` bitmap.
  - **Shield**: defined by `SHIELD_SPELL` bitmap.
  - **Stun**: defined by `STUN_SPELL` bitmap.

The LCD shows:
- Mage HP, Shield,
- Mole HP, Shield,
- Remaining pixels for the current turn.

---

## Gameplay Overview

### Map & Exploration

- The world is a fixed **16×16 grid** with walls and floors (`rawMap` → `mapData`).
- The **LED matrix** shows an **8×8 viewport** into this world:
  - `viewX`, `viewY` = top‑left tile of the viewport.
  - `drawViewport()` draws walls from `mapData` in this 8×8 window.
  - `drawPlayerAndMoles()` draws player, moles, potions on top.

- The **camera** follows the player:
  - `viewX = playerGX - 4`, `viewY = playerGY - 4`
  - clamped so the viewport never goes outside the 16×16 boundaries.

- **Moles** (enemies) and **potions** (heals) are placed at hardcoded valid positions (floors, not overlapping each other or walls).

### Potions

- Stepping on a potion tile:
  - potion is consumed,
  - mage heals +8 HP, clamped to `MAGE_MAX_HP`,
  - a short **non‑blocking LCD message** “heal potion picked & used” is shown,
  - during this message, the HUD is not redrawn to avoid flicker.

### Combat Trigger

- `isNearMole()` checks a 3×3 area around each alive mole:
  - if the player is adjacent (8 neighbors, but not on the same tile),
  - sets `currentMoleIndex` to that mole and returns `true`.
- In `loopExplore()`, if `!escapeCooldownActive && isNearMole()`:
  - calls `enterCombatMode()` and switches to `STATE_COMBAT`.

---

## Combat System

### Stats & Flow

- Mage:
  - `MAGE_MAX_HP = 20`
  - Attack: 8 damage
  - Shield: 4 block
- Mole:
  - `MOLE_MAX_HP = 18`
  - Attack: 6 damage
  - Shield: 6 block
- `mageShield` and `moleShield` absorb damage once, then reset to 0.

Combat is **turn‑based**:

1. **Mage turn**:
   - player draws up to `pixelsPerTurn` pixels on the 8×8 matrix.
   - joystick button attempts to cast a spell:
     - matches `ATTACK_SPELL` → attack,
     - matches `SHIELD_SPELL` → shield,
     - matches `STUN_SPELL` → stun,
     - otherwise: no effect.

2. After a valid spell:
   - if mole’s HP ≤ 0 → win.
   - otherwise `moleTurn()` executes (unless overridden by stun logic).
   - then a short **enemy turn phase** shows “Mole attacked / Mole shielded / Mole is stunned” for a brief time (non‑blocking), and control returns to mage.

### Spells

#### Attack

- Bitmap: `ATTACK_SPELL[8]` (8×8 pattern).
- When recognized:
  - `applyMageAttack()`:
    - damage = 8,
    - reduced by `moleShield`,
    - mole’s HP clamps at 0,
    - `moleShield` set back to 0.
  - Plays an attack animation (non‑blocking) and a buzzer sound.

#### Shield

- Bitmap: `SHIELD_SPELL[8]`.
- When recognized:
  - `applyMageShield()`:
    - `mageShield = 4` (or `+= 4` if you choose stacking),
  - Starts a shield animation and shield sound.

- Mole attacks are reduced by `mageShield`, then `mageShield` resets to 0.

#### Stun

- Bitmap: `STUN_SPELL[8]`.
- When recognized, the game simulates what the mole would do this turn:
  - a random roll decides **plannedAction** ∈ {attack, shield} based on `MOLE_ATTACK_CHANCE`.

- If **plannedAction = SHIELD**:
  - Stun **succeeds**:
    - `moleShield = 0`,
    - `moleStunTurns = 2` (next 2 enemy turns are skipped),
    - `lastMoleAction = MOLE_ACT_NONE` interpreted as stunned,
    - LCD: “Stun success / Mole stunned”,
    - Stun success sound is played,
    - enemy turn phase only shows that the mole is stunned.

- If **plannedAction = ATTACK**:
  - Stun is **ignored** for this turn:
    - `moleTurn()` runs as a normal attack,
    - a stun fail sound is played,
    - enemy turn phase shows “Mole attacked”.

The stun thus has a conditional, high‑impact effect that interacts with the enemy’s intended behavior, adding risk–reward to using it.

---

## Escape Mechanic (HC‑SR04)

The HC‑SR04 adds a **“panic escape”** mechanic:

- If you bring your hand **very close** to the sensor at the **start of the first combat round**, you “arm” a one‑shot escape:
  - `escapePossibleThisCombat = true`,
  - if distance `< threshold` (e.g. 8 cm) once, `escapeRequested = true`.

Design rules:

1. You cannot flee immediately:
   - the **first full round** must still be played:
     - you cast a spell,
     - mole gets its turn (may deal damage).
2. At the end of the **first enemy turn**:
   - if `escapeRequested && escapePossibleThisCombat`:
     - combat ends with `performCombatEscape()`,
     - you return to explore mode,
     - the mole’s HP & status are reset for the next encounter.
   - If you didn’t request escape, `escapePossibleThisCombat` is set to false and you can no longer flee in that combat.

To avoid re‑entering combat immediately (because you’re still near the mole), there is an **escape cooldown**:

- `escapeCooldownActive = true` for a short duration in explore mode.
- During cooldown, `isNearMole()` is ignored when deciding to start a new combat.
- After the cooldown, you can re‑engage the same mole, but its stats are fresh.

This creates a trade‑off: escape can save you from a lethal mistake but always costs you at least one enemy turn (and possibly HP), and you must re‑fight the mole from scratch.

---

## Non‑Blocking Architecture

One of the main goals of the project is to avoid `delay()` and keep the game responsive. The gameplay is implemented as a **state machine** using `millis()` timers:

- **Game states** (`gameState`): INTRO, TUTORIAL, MENU, EXPLORE, COMBAT, PAUSE, WIN, LOSE.
- Each `loopX()` function handles one state (e.g. `loopExplore()`, `loopCombat()`).
- Timed sequences (intro texts, tutorial, enemy turn messages, animations) use:
  - an enum for sub‑states,
  - `unsigned long stateStartTime`,
  - `if (millis() - stateStartTime >= duration)` to progress.

Examples:

- Tutorial intro:
  - sequences of texts and the spell hint are implemented with `TutorialIntroState` and `tutorialIntroStateStart`.
- Enemy turn:
  - `enemyTurnActive`, `enemyMessageActive`, `enemyTurnStartTime`, `enemyMessageStartTime` control delays between enemy action and message display without blocking the main loop.
- Attack / shield animations:
  - small internal animation states (`AttackAnimState`, `ShieldAnimState`) updated each frame via `updateAttackAnimation()` / `updateShieldAnimation()`.

This design allows:
- continuous polling of inputs,
- multiple “timers” (animations, messages, blinking cursors, escape cooldown) running in parallel on a single‑threaded microcontroller.

---

## Code Structure (High Level)

- **Main loop & setup**
  - `setup()`: initializes matrix, LCD, joystick, map, buzzer.
  - `loop()`: dispatches to state handlers based on `gameState`.

- **Rendering**
  - `drawViewport()`: draws walls in the 8×8 camera window.
  - `drawPlayerAndMoles()`: draws mage, moles, potions.
  - `updateLCDGameExplore()`: shows HP and number of moles killed.
  - `updateLCDGameCombat()`: shows HP/shield/pixels left in combat.

- **Input**
  - `handleJoystickMoveExplore()`: grid movement, collision checking.
  - `handleJoystickMoveCombat()`: cursor movement on 8×8.
  - `handleCombatButtons()`: drawing / clearing pixels.

- **Spells & Combat**
  - `spellMatchesBitmap()`, `isSpellAttack()`, `isSpellShield()`, `isSpellStun()`.
  - `applyMageAttack()`, `applyMageShield()`, `moleTurn()`.
  - `loopCombat()`: full state machine for turns, enemy actions, spells, stun, escape.

- **World & Encounters**
  - `initMap()`: mapData, mole positions, potions.
  - `isNearMole()`: checks adjacency to moles.
  - `enterCombatMode()`, `exitCombatModeWin()`, `exitCombatModeLose()`, `performCombatEscape()`.

- **Tutorial & Intro**
  - `loopIntro()`: story text in multiple timed pages.
  - `loopTutorialIntro()`: teaches basic attack spell with non‑blocking hint and success/fail messages.

---

## Design Philosophy

### 1. Clarity over complexity

The game is built as small, clear state machines instead of nested `if` + `delay()` chains.  
Each state has its own `loopX()`, and timed events use `state + timestamp`, which makes behavior easier to understand and debug on hardware.

### 2. Risk–Reward Combat

Attack, Shield, Stun, and Escape are designed to force choices, not button‑mashing.  
Attack is strong, Shield trades a turn for safety, Stun is powerful but situational, and Escape always costs at least one enemy turn and a full reset of the fight, pushing the player to think about enemy intent and timing.

### 3. Hardware as Game Mechanic

Core mechanics are mapped directly to hardware:  
the 8×8 matrix for rune‑drawing spells, the HC‑SR04 for a physical “panic escape” gesture, and the buzzer for instant feedback.  
Limited I/O becomes part of the game design rather than a constraint.

### 4. Non‑blocking Flow

Using `millis()` instead of `delay()` keeps the loop responsive, allows multiple parallel timers (animations, messages, cooldowns), and makes it easier to add new effects without freezing the rest of the game.
