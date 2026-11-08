# Crimson Desert modding research: stamina and combat state

Notes as of November 2026 (game 2.04.x, after the *Charting the Unknown* DLC).

## 1. There is no official modding API

Pearl Abyss has released no SDK or official mod support. Everything runs on
community tooling:

| Layer | Tools | What it can do |
|---|---|---|
| Data / archive patches | JSON Mod Manager (PhorgeForge's byte-patch format), CDUMM, DMM, Crimson Desert Mod Loader (virtual PAZ) | Change static game data in `.pabgb` tables inside PAZ archives, e.g. `skill.pabgb` stamina costs |
| Runtime code (ASI) | Ultimate ASI Loader plus your own DLL, usually with SafetyHook | Hook game functions, read and write live memory |
| Cheat Engine | FearLess / OpenCheatTables tables | Pointers to PlayerBase HP and stamina, good for finding hook sites |
| Small C APIs | World Builder exposes `cdmodkit_api.h` for spawning objects | Nothing about stats |

## 2. Why every existing stamina mod ignores combat

- **Data mods** (Infinite Stamina #107, Stamina & Spirit consumption, Crimson
  Stamina Tool, Traversal Unbound #3563) edit the `UseResourceStat` entries
  under `skill._useResourceStatList`. They are static numbers. They cannot know
  whether you are in a fight.
- **Traversal Unbound** is the closest existing mod. It cuts costs only for
  traversal skills (climb, glide, swim) and leaves attacks alone. But it is
  still per skill, not per situation, so sprinting or rolling in a fight costs
  the same as out of one.
- **stamina-master** (shin2344234, ASI, MIT) checked every skill field
  (`_cooltime`, `_applyType`, `_damageType`, `_uiType`, ...) and found that
  **the game's data does not separate movement skills from combat skills**.
  Climbing shares every value with attacking. So a data-level combat split is
  a dead end.

**Conclusion:** "No stamina outside combat" has to be a **runtime** mod that
checks game state at the moment stamina changes.

## 3. Hook site: shared stat-write commit

Found by Orcax-1399 (`CrimsonDesert-player-status-modifier`, `scanner.cpp`
`ScanForStatWriteAccess`), and reused by the CrimsonDesertCoop project:

```
48 2B 47 18 48 39 5F 18 48 0F 4F C2 48 89 47 20 48 FF 47 48
48 89 5F 08            <- hook here: mov [rdi+8], rbx  (offset 20)
48 8B 5C 24 48 48 89 77 38 66 89 6F 50
```

- `rdi` is the stat entry and `rbx` is the new value about to be stored.
- Entry layout: `+0x00` int32 stat id (0 Health, 17 Stamina, 18 Spirit),
  `+0x08` int64 current, `+0x18` int64 max.
- Health, Stamina and Spirit for every actor pass through here. Orcax filters
  to the player by tracking the player's entry pointer from a second hook.
- Orcax's own notes (`research-note-stamina.md`) say they tried hooking
  separate "instant" and "continuous" stamina paths, could not get stable
  results, and went back to this commit point. That is why this mod uses it.
- Units: sprinting is about -8000 per tick, climbing -10000, a roll -20000, an
  air dash -30000.

**Version risk:** these patterns come from the April 2026 builds.
stamina-master's `signatures.h` documents that the 2.04.00 DLC build (exe
1.0.0.3392) moved most code. This site is not confirmed on 2.04.x. It needs one
in-game run to check.

## 4. Combat state: what is known

| Candidate | Source | Status |
|---|---|---|
| `48 8B 05 ?? ?? ?? ?? 80 78 1A 01`, byte at +0x1A | JustSkip README / INI | **An example only.** JustSkip says "if the community discovers the game's in-memory combat flag, paste an AOB". CrimsonDesertCoop lists it as "Verified", but only cites JustSkip. JustSkip's resolver also does not dereference the global, so even the example's semantics are unclear |
| Evaluator `+0x6A` flag | CDAnimCancel, via CrimsonDesertCoop | Per action, not "in combat". Source repo is 404 |
| Actor base `+0x130 / +0x131` | CrimsonDesertCoop | Marked deprecated |
| JustSkip signal fusion | vibration, lock-on + attack, button mashing | Works, heuristic |

So this mod uses **signal fusion**, with a stronger signal than JustSkip's:
any actor's Health dropping, seen at the same stat-write hook, plus attack
inputs. The optional `FlagPattern` keeps a slot for a real flag once someone
confirms one.

### Finding a real combat flag (for later)

1. Cheat Engine, byte scan, value unknown.
2. Stand still out of combat and scan "unchanged". Aggro an enemy and scan
   "changed". Wait out of combat until the music stops and scan "changed".
   Repeat.
3. Good candidates also flip with the combat music or HUD, not with your
   inputs.
4. *Find out what accesses* the byte, take a stable `mov rax,[rip+X]` /
   `cmp byte [rax+N],1` style instruction, and put it in `FlagPattern` /
   `FlagByteOffset`.

## Sources

- Nexus: [Infinite Stamina #107](https://www.nexusmods.com/crimsondesert/mods/107),
  [Traversal Unbound #3563](https://www.nexusmods.com/crimsondesert/mods/3563),
  [Infinite Stamina and Spirit #2581](https://www.nexusmods.com/crimsondesert/mods/2581),
  [Crimson Stamina Tool #2527](https://www.nexusmods.com/crimsondesert/mods/2527),
  [JSON Mod Manager #113](https://www.nexusmods.com/crimsondesert/mods/113),
  [CDUMM #207](https://www.nexusmods.com/crimsondesert/mods/207),
  [DMM #633](https://www.nexusmods.com/crimsondesert/mods/633),
  [Mod Loader #3527](https://www.nexusmods.com/crimsondesert/mods/3527),
  [World Builder #3546](https://www.nexusmods.com/crimsondesert/mods/3546)
- GitHub: [shin2344234/stamina-master](https://github.com/shin2344234/stamina-master),
  [Orcax-1399/CrimsonDesert-player-status-modifier](https://github.com/Orcax-1399/CrimsonDesert-player-status-modifier),
  [blizz3010/CrimsonDesertCoop](https://github.com/blizz3010/CrimsonDesertCoop),
  [wealdly/JustSkip](https://github.com/wealdly/JustSkip),
  [XeTrinityz/Trinity](https://github.com/XeTrinityz/Trinity)
- Cheat tables: [FearLess thread](https://fearlessrevolution.com/viewtopic.php?t=38679),
  [OpenCheatTables](https://opencheattables.com/viewtopic.php?t=1836)
