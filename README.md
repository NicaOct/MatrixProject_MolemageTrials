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

***
## Showcase

![1a](https://github.com/user-attachments/assets/565df2c6-26a3-4bcf-9333-5c7a8eac5def)
![2a](https://github.com/user-attachments/assets/5bda1610-de58-4da1-8d5f-0f44a3d5975a)
![3a](https://github.com/user-attachments/assets/c35d0e63-7b2e-4af9-bb11-03d3eeb14426)
![4a](https://github.com/user-attachments/assets/453d5ab6-dbff-408c-a7bc-d66316ca5838)


[Video](https://youtu.be/qKX8waVqMMQ)

***
