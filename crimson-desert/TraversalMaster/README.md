# Traversal Master (Crimson Desert ASI mod)

Traversal Unbound's behaviour, rebuilt so game updates don't break it.

- Cheaper (or free) stamina for **climbing, swimming, gliding / shield glider
  / rocket pack, and mounted actions**. It uses the same 52 skills Traversal
  Unbound 1.1.0 lists.
- Combat, rolls, sprinting and everything else keep the **game's own cost**.

## Why Traversal Unbound broke and this doesn't

Traversal Unbound is a DMM "format 2" patch. It writes new bytes at fixed
**offsets** inside `gamedata/skill.pabgb` (for example offset `370390` for
`Skill_Mount_Ing`). Every game update that adds or changes a skill row shifts
those offsets, so the patch either fails or would write over the wrong data.

Traversal Master patches no game file. After the game loads its skill table,
the plugin finds each skill **by name** (`Skill_PointClimbLv0`,
`Skill_CrowWing`...) and scales that skill's stamina cost in memory. Patches
that shift the file don't matter. It only breaks if Pearl Abyss changes how the
table itself is laid out in memory, and then it logs the failure and changes
nothing.

It's a fork of [Stamina Master](https://github.com/shin2344234/stamina-master)
by Seth (MIT, upstream commit `44a4a68`, version 1.0.7, built for **2.04.08 /
exe 1.0.0.3576**). All of the table-reading code is Seth's. This fork adds the
traversal-only filter (`TraversalOnly`, `TraversalPercent`, the skill list).
See `UPSTREAM_README.md` and `LICENSE`.

## Settings (`TraversalMaster.ini`)

| Key | Default | Meaning |
|---|---|---|
| `TraversalOnly` | `1` | `1` = only the listed traversal skills change. `0` = plain Stamina Master behaviour |
| `TraversalPercent` | `25` | traversal cost as a % of vanilla. `25`/`50`/`75` = Traversal Unbound's variants, `0` = its infinite variant |
| `TraversalExtraSkills` | *(empty)* | more skill keys, comma separated, e.g. `Skill_BasicMoveLv4` for sprinting |
| `TraversalExcludeSkills` | *(empty)* | default skills to drop, e.g. `Skill_Climb_DaggerAttack,Skill_Climb_DaggerChargeAttack` |
| `MountRegenPercent` | `400` | horse stamina recovery, 100 to 10000 (Traversal Unbound's 25% variant was about 400) |
| `SpiritPercent` | `100` | Spirit costs, untouched by default |
| `Probe` | `0` | `1` = log a line for every skill changed, and the research report |

Settings are read once at startup, so restart the game after you edit them.

## Install with DMM

1. **Disable Traversal Unbound** (and any other stamina JSON mod) in DMM, then
   apply.
2. Build on Windows (MSVC Build Tools 2022): `package.bat`. It produces
   `dist\TraversalMaster-DMM.zip` (`bin64\TraversalMaster.asi` +
   `bin64\TraversalMaster.ini`).
3. Import the zip into DMM, enable it, apply.
4. Start the game, then open `bin64\TraversalMaster.log`. You want a line like
   `[traversal] traversal-only mode: 52 of 52 listed skill(s) found ... at 25%`.
   - The DMM install is sometimes only the `.asi`. If so, the plugin writes a
     default `TraversalMaster.ini` beside itself on first run.
   - If fewer than 52 skills are found, the game renamed some of them. Set
     `Probe=1` and the log shows the current names.

Don't run it together with Stamina Master. They rewrite the same numbers.

## Known limits (from upstream)

- Blocking a hit still costs stamina. The game charges that outside the skill
  table.
- Not tested in game by me. The upstream table code is what Stamina Master
  ships for 2.04.08.
