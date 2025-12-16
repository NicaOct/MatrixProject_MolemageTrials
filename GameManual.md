# Molemage Trials – Game Manual

## Premise

You wake up in a twisted, maze‑like dungeon with no memory of what came before, armed only with a dim staff, a spellbook, and your wits. By tracing glowing runes on a magical 8×8 grid, you cast spells to fight burrowing moles that stalk the corridors. Potions scattered through the labyrinth keep you alive, while your spell choices and timing decide whether you control the battle, barely survive it, or desperately flee to fight another day.

---

## Controls

### Joystick

- **Move in Explore Mode**:  
  - Tilt up/down/left/right to move the mage one tile at a time on the map (walls block movement).
- **Move Cursor in Combat**:  
  - Tilt to move the drawing cursor around the 8×8 LED matrix.
- **Joystick Button**:  
  - In menus: select / confirm.  
  - In combat: cast the spell drawn on the matrix (if it matches a valid rune).

### Buttons

- **DRAW Button**:
  - In combat: sets a pixel at the cursor position, up to a per‑turn limit (`pixelsPerTurn`).
- **CLEAR Button**:
  - In combat: clears all pixels drawn this turn, letting you redraw the rune.
- **PAUSE Button**:
  - In explore/combat: opens the pause menu (Resume, Show Spells, Exit).

### Ultrasonic Sensor (HC‑SR04)

- **Escape Gesture**:
  - At the start of a battle, bring your hand very close to the sensor once to “arm” an escape.
  - If escape is armed, after the first full round (your spell + mole’s turn), you will automatically flee the combat:
    - you return to the map with your current HP,
    - the mole’s HP and status are reset for the next encounter,
    - a short cooldown prevents you from instantly re‑entering combat.

---

## Spells & Combat

### Drawing System

- Combat happens on an **8×8 LED matrix**.
- Each turn you may light up to `pixelsPerTurn` pixels using the DRAW button.
- The game compares your drawn pattern to predefined 8×8 bitmaps:
  - **ATTACK_SPELL**
  - **SHIELD_SPELL**
  - **STUN_SPELL**
- If the pattern matches:
  - the corresponding spell is cast and the turn proceeds,
- If not:
  - nothing happens and the turn is effectively wasted.

### Mage Stats & Spells

- **Stats**:
  - Maximum HP: `MAGE_MAX_HP = 20`
  - Attack damage: 8
  - Shield value: 4

- **Basic Attack Spell**:
  - On hit:  
    - Deals 8 damage minus current mole shield.  
    - Mole HP cannot go below 0.  
    - Mole shield is reset to 0.
  - Visual feedback: attack animation + buzzer tone.

- **Shield Spell**:
  - On cast:
    - Sets mage shield to 4 (or adds 4 if stacking is enabled in code).
    - Incoming mole damage is reduced by this shield, then shield resets to 0.
  - Visual feedback: border animation + shield sound.

- **Stun Spell**:
  - The game “rolls” what the mole intended to do this turn:
    - If the mole planned to **shield**:
      - Stun **succeeds**:
        - Mole shield is cleared,
        - `moleStunTurns = 2` (its next two turns are skipped),
        - Enemy turn displays “Mole is stunned”.
      - You effectively lock the mole out of its next few actions.
    - If the mole planned to **attack**:
      - Stun is **ignored** for that turn:
        - The mole attacks normally,
        - Enemy turn displays “Mole attacked”.

This makes Stun a high‑risk, high‑reward spell based on enemy intent.

### Mole Stats & Behaviour

- **Stats**:
  - Maximum HP: `MOLE_MAX_HP = 18`
  - Attack damage: 6
  - Shield value: 6

- **Turn Logic**:
  - On its turn, the mole randomly chooses (based on `MOLE_ATTACK_CHANCE`):
    - **Attack**:
      - Deals 6 damage minus mage shield.
      - Mage HP cannot go below 0.
      - Mage shield resets to 0 after absorbing damage.
    - **Shield**:
      - Sets its shield to 6, which reduces damage from the next mage attack.

- **Stun Interaction**:
  - If `moleStunTurns > 0`, the mole skips its turn entirely:
    - No attack, no shield.
    - `moleStunTurns` is decremented.
    - Enemy turn message shows “Mole is stunned”.

---

## Objective of the Game

- **Explore the Dungeon**:
  - Navigate the 16×16 maze, avoid or engage moles, and search for heal potions.
- **Survive the Fights**:
  - Use Attack, Shield, Stun, and Escape intelligently.
  - Manage your HP across multiple combats; potions are limited and your health does not fully reset between battles.
- **Defeat All Moles**:
  - Each mole you defeat is permanently removed from the map.
  - When all moles are dead, you win the game.
- **Avoid Death**:
  - If your HP reaches 0 at any point, you die and the game shows a lose screen.

The core challenge is to balance aggression and defense, read the enemy’s possible actions, and decide when to risk a Stun or trigger an Escape, all while navigating a tight hardware‑constrained environment.
