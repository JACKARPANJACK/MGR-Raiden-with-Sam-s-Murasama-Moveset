Current update: see [DIRECTIONAL_MOVES.md](DIRECTIONAL_MOVES.md). The directional add-on implementation supersedes older ultimate, sheath, and charge notes below.

# Combat replacement routing repair — 2026-09-30

This section supersedes the notes below.

The previous ASI could report activation without replacing any clips. Its
cFmerge filename hook missed the hashed motion lookup used by Animation::findMotion
(0xA355E0). It also compared archive object addresses, although GetSequenceFile
(0x696360) uses a local copy of Behavior::m_DataFile. The campaign prefix pl0010
was rejected by the old filename filter.

The rebuilt ASI hooks those two actual lookup functions. During an active-player
native requestAnimationByName call, findMotion selects a complete Sam clip;
GetSequenceFile supplies that exact clip's sequence for the same actor/code.
Missing pairs use both native files. The engine performs normal slot setup once,
keeping native request parameters, map flags and action bookkeeping. Requests for
other actors, locomotion, reactions and executions are excluded.

The existing combat allowlist has 27 codes. Local stock archives contain 26
complete Sam pairs (25 playable, one boss); 26b0 has no complete pair and remains
native. Release/replacement-routing-verification.json records each filename.
No archive files are rewritten. Explicit loose-file resource loading was removed;
resources stream through the game's loader and are pinned only after completion.
Effect/sound initialization waits for every requested resource.

Automatic direct charge/finisher playback was still running despite the prior
freeze-repair notes. It is now disabled in OnTick, so it cannot interrupt these
native paired requests. Raiden retains his controller: this change supplies Sam
combat clips and their event sequences, not Sam's full DLC state machine.

Diagnostics show motion substitutions and a separate count of Sam sequences
actually supplied. The log records SEQUENCE supplied lines. Activation alone is
not evidence that a combat request has been replaced.

Validation: Release Win32 build; combat routing tests include the native short
code and prefixed stem formats, exact paired sequence selection, mismatched
codes, missing sequences and native locomotion; archive resolver and legacy
style tests pass. Binary disassembly confirmed the native lookup call paths.
The rebuilt ASI is installed with a backup and SHA-256 verification. The game
was closed at installation. Full restart and G activation are needed.

Gameplay has not been verified. Check light/heavy chains, air attacks, dodge,
parry, hit timing, G on/off and checkpoint reload. The diagnostics' sequence
count should increase during replaced combat requests.

---

## Previous notes (superseded)

# Toggle freeze repair — 2026-09-29

This section supersedes the historical implementation notes below.

The G-enabled path mixed Raiden's native action handler with Sam-only action
states, global attack-ID/vtable patches, and per-frame Sam state processing.
It also reissued animations with setDirectAnimation after the native request,
overriding the map flags and potentially restarting playback twice through
nested map/name hooks.

The repaired build keeps the native Raiden controller. It removes those global
patches, Sam-tail writes, DLC action/event/sheath/enemy hooks and shared map
renaming. The factory allocation is unchanged. G changes clip-routing state;
it no longer changes character dispatch or interrupts the current action.

Animation replacement occurs only inside native animation requests for the
active player, through that player's archive lookup. Known 2xxx combat clips
use a complete motion/sequence pair from playable Sam; missing pairs fall back
to native files. Locomotion, executions and hit reactions remain native. Native
map flags, playback parameters and slot initialization stay with the engine.

This repair disables experimental Sam charge logic, boss enders, model-ID
patches and forced sheath positioning. It is not a full native Sam controller
port. Combat timing and animation compatibility still require in-game testing.
No game archives are changed.

Validation: Release Win32 build; combat routing tests (file types, sequence
channels, excluded locomotion, actor names and malformed names); archive lookup
tests; existing StyleSwitch tests (StyleSwitch is no longer used by activation).
Full restart required when changing from the previous ASI. Playtest G on/off
while idle, moving and attacking, then air attacks, dodge and checkpoint reload.

---

## Historical notes (superseded)

# Runtime Sam controller work â€” 2026-09-28

**Experimental runtime transfer.** G requests resources, expands future Raiden
allocations to Sam's verified `0x5470` size, swaps the player to Sam's verified
vtable, runs Sam startup, and routes direct action calls through Sam's handler.
The ASI was built locally, not installed into the game.
The source snapshot before these changes is in `backups/runtime-resource-20260928`.

## Implemented and compiled

- Request `pl1400`, `em0020`, `pl1403` and `pl1404` through the engine loader.
- Wait for all four resources and retrieve separate archive containers for both
  playable and boss Sam. Retain use references for animation-pointer lifetime.
- Resolve complete motion/sequence pairs from the same archive. Callers can
  explicitly select boss Sam, disable fallback, or allow sequence-less clips.
  DLC channel 2 and boss channel 0 take priority; channel 3 is also supported.
- Remove the explicit loose `GameData/pl/pl1400.dat` fallback. No game archives
  are rewritten or extracted by this code. The game's own loader may still
  prefer existing loose mods; this change does not remove those files.
- Poll toggle input once per tick, with a foreground-process check.
- Restore the DLC initialization mode even if the initialization function faults.
  That function is called only after all requested archives are loaded.
- Preserve the original Sam sheath handler for unrelated actors. Avoid global
  model/routing writes while the moveset is inactive.
- Resolve player animation-map requests through the loaded Sam archive at
  `Behavior::requestAnimationByMap`, preserving Raiden's map metadata while
  supplying Sam motion and sequence data. The hook is restricted to the active
  player and falls back to the original engine request if a clip is missing.
- Drive Sam charge stance/air-charge states from heavy-button hold duration,
  tracking the 15/45/75-frame tiers and leaving release transition handling to
  Sam's native state machine.
- Reinitialize the Sam tail and animation routing after a checkpoint/player
  respawn.
- Add four rotating boss-Sam combo enders (`2510`, `2520`, `2530`, `2540`).
  After the native three-slash chain reaches its completed state, pressing
  heavy starts the selected `em0020` motion and sequence directly, preserving
  its attack and effect tracks; the original combo flags are then reset.
- Disable automatic post-build installation.

## Runtime limitations and risk

The SDK validates `sizeof(Pl0000) == 0x5400` and `sizeof(Pl1400) == 0x5470`.
IDA confirmed the two `0x5400` immediates in the Raiden factory, and the ASI
patches them to `0x5470` before any player is allocated. Existing players from
before DLL attach cannot be safely upgraded; restart the game after replacing
the ASI. In-game playtesting is still required for the vtable transition,
checkpoint reload, and toggling during active animations.

`StyleSwitch` still uses 48 renamed animation aliases in Raiden's own archive.
Those aliases originate from the old external-asset workflow; the new resource
loader is not yet a replacement for the engine's animation/map/sequence lookup.
Loading archives alone does not implement hit detection, native charge state,
sheath transitions, boss effects, or synchronized enemy executions.

## Remaining work for the requested result

1. Route action maps, motions and sequences to the selected archive at runtime,
   and validate bone and sequence-event compatibility with Raiden's rig.
2. Port charge/release, sheath, quickdraw, effects/audio registration and paired
   finisher transitions, retaining campaign event and damage interruption rules.
3. Capture and restore every modified dispatch/code value, including checkpoint,
   respawn and G toggles during active animations.
4. Test in game against unmodified archives and with other installed plugins.

## Validation

Release Win32 builds with the configured SDK. `tools/test-resources.cmd` exercises
the production archive resolver, including overlapping codes, missing sequences,
explicit boss selection and invalid names. `tools/test-style.cmd` covers the
existing private animation-map lifetime and restoration behavior. These tests
do not establish game-engine loading, combat, effects or finisher correctness.
