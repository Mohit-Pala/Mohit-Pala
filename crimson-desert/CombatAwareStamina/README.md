# CombatAwareStamina (Crimson Desert ASI mod)

Free (or cheaper) stamina **outside combat**, vanilla stamina **in combat**.

Every stamina mod on Nexus right now is either a JSON/`.pabgb` data patch or a
blanket memory hook, so it applies everywhere. This one hooks the game's
stat-write commit at runtime and decides **on each stamina change** whether
you are fighting.

> **Status: untested in game.** It is written against hook sites other
> open-source mods published for the April 2026 builds. Game 2.04.x (the DLC
> build) moved a lot of code, so the scan may fail on your version. When it
> does, the mod logs it and does nothing. It will not patch the wrong place:
> it only hooks if the pattern is unique *and* the bytes there are the exact
> store it expects. See [docs/RESEARCH.md](docs/RESEARCH.md).

## How "in combat" is decided

There is no verified in-memory "in combat" flag yet (see research notes), so
the mod combines signals. If any of these fired in the last `TimeoutMs`
(default 6 s), you count as in combat:

| Signal | Source |
|---|---|
| Any actor loses health (you get hit, an enemy gets hit) | the same stat-write hook, Health stat id 0 |
| You press an attack / guard input | `AttackKeys` (keyboard/mouse VK codes) and `GamepadButtons` (XInput mask) |
| *(optional)* a game-side combat byte | `FlagPattern`, off by default |

Outside combat, stamina drains are scaled by `OutOfCombatCostPercent`
(default **0 = no stamina**). Inside combat they are scaled by
`InCombatCostPercent` (default **100 = vanilla**). You can also speed up
regen out of combat with `OutOfCombatRegenPercent`.

**F8** toggles the mod in game (high beep = on, low beep = off).

## Install

1. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
   into `Crimson Desert\bin64\`. Most Crimson Desert ASI mods already need it,
   so you may have it.
2. Copy `CombatAwareStamina.asi` and `CombatAwareStamina.ini` into `bin64\`.
3. Start the game and check `bin64\CombatAwareStamina.log`. You want to see
   `hooked stat-write at ...`.

Do not run it next to another mod that rewrites player stamina at runtime,
such as Orcax's player-status-modifier, Trinity's infinite stamina, or a
Cheat Engine table. Data-patch mods (JSON / `skill.pabgb`) stack with it, but
then the "in combat" costs are their reduced costs, not vanilla.

## Build (Windows)

Needs Visual Studio 2022 Build Tools (MSVC) and CMake 3.21+. SafetyHook (and
Zydis) are pulled in by CMake `FetchContent`.

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
:: -> build\Release\CombatAwareStamina.asi
```

## After a game patch

If the log says `stat-write hook site not found`:

1. In Cheat Engine, find your current stamina (8-byte int). Use
   *Find out what writes to this address*, then sprint.
2. One of the writers is the shared commit, `mov [rdi+08],rbx`. It also fires
   for Health and Spirit.
3. Copy about 20 unique bytes around it into `StatWritePattern`, and set
   `StatWriteOffset` to the index of the `48 89 5F 08` byte. If the register or
   offset changed, the code needs a small edit too. Look for
   `mov [rdi+0x08], rbx` in `src/dllmain.cpp`.

## Known limitations

- **Horse stamina** seems to go through a different code path ("AB00" in
  Orcax's notes). It may not be affected.
- **Spirit** is not touched, only Stamina (stat id 17).
- The mod changes stamina for **every actor** while you are out of combat, not
  only yours. Nobody near you is fighting at that point, so in practice this
  doesn't matter, and it avoids a fragile, version-specific player-pointer
  chain.
- The first swing that starts a fight costs vanilla stamina only if the input
  signal sees it first. Inputs are polled every 10 ms, so this is usually fine.
- Fall damage, or an NPC getting hit near you, counts as combat for a few
  seconds. That errs on the vanilla side.
