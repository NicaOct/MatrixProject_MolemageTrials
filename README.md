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



