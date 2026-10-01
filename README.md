# Raiden with Sam's Murasama Moveset

Win32 ASI plugin for Metal Gear Rising: Revengeance. G enables the native Sam DLC
combat controller on Raiden, including charge/Iai, Blade Mode and Datsu nodes.
Non-2xxx DLC animations now route to Sam's motion and sequence assets, including
finishers and Zandatsu. Seven execution clips intentionally use shared native
motions with their Sam DLC sequences. Scripted story states retain native routing.

## Controls

- **G**: toggle Sam's sword moveset.
- **0**: open the menu, including the weapon selector.
- **Q / E** (also **[ / ]**): select previous / next weapon.
- **Mouse wheel Up / Down**: previous / next weapon, including Murasama and Unarmed.
- **D-pad**: native inventory, including the heatknife.
- **F7 / F8**: previous / next secondary mode; also selectable in the menu.
- **C / controller subweapon button** with Bladewolf heatblades selected: throw.
- **F** in Sam mode: sweeping finisher against a nearby weak cyborg.
- **X** or **controller B / Circle** in Sam mode: queue the next lightning ultimate.
- **Select / Back** or **L3 + R3**: toggle Sam's moveset.
- Directional Light/Heavy and flick inputs add boss attacks; see
  [directional controls](DIRECTIONAL_MOVES.md).

Weapons: Raiden sword, Murasama, Pole-arm, Sai, Pincer blades, Unarmed and Bladewolf heatblades. Switching waits
for a safe ground state and waits for streamed weapon assets before equipping.
Selected and pending weapons stream on demand. Mouse-wheel
notches accumulate, so precise wheels work too; rapid requests select the latest
weapon without replaying each intermediate swap. Sam's ordinary ground attacks
allow a swap in their final six recovery frames. Blade Mode, charge holds,
Round Trip, ultimates, aerial and scripted actions keep their transition guards.
Menu scrolling stays with the menu. B is reserved for ultimates during Sam combat;
native QTE/context inputs remain available outside those states.
Unarmed equips custom weapon ID 5 and mounts `wp2040`, including its native
unarmed motion/sequence, effect and sound assets. The sword and sheath stay
attached; switching never calls the sword-lost/drop path. Enemy-contact slow
motion now skips absent optional weapon objects safely.
Selecting Murasama enables Sam's moveset; selecting another melee weapon disables it,
including a pending Sam activation. Q/E cycles through Murasama as
a separate choice. Heatblades preserve Sam's moveset. Melee secondary weapons and unarmed combat use Raiden's native
controller. G remains a synchronized Sam toggle. Switch inputs are reserved
during normal gameplay; scripted/QTE input stays native. Native finishers/Zandatsu use
their normal in-game prompts; F is a separate combat finisher.

Kunais use Raiden's native knife inventory. **Hold C / controller subweapon**
for camera-based precision aiming and release to fire. Mouse/right-stick camera
controls aim the reticle. The charge HUD shows payload, knives and target count:

- Tap: the payload selected with F7/F8 (heat, stun or explosive).
- Hold 18 gameplay ticks (about 0.3s): stun knife.
- Hold 45 ticks (about 0.75s): explosive knife.
- Reach 90 ticks (about 1.5s): automatically fire up to ten knives in a 90-degree
  fan, using the selected payload. Release C before starting another charge.

Charged knives track moving enemies; the volley distributes knives across up to
three targets around the reticle. If fewer targets exist, knives share a target.
No target means a fan centered on the held reticle ray. Fan knives travel outward
for 18 ticks before enemy homing begins. Homing retains native collision and ends
after 120 projectile ticks. Every successful spawn consumes one native knife;
a volley is limited by ammunition and active-projectile capacity. Stun applies
to ordinary enemies and heat applies bounded damage over time. Explosive knives
spawn the native grenade payload on impact or projectile timeout.

Throws work during ground/air combos, movement and Blade Mode. Neutral ground
held-C aiming uses Raiden's native `2561` motion at 2.5x speed with a five-frame
entry blend. Throws blend directly into `2566` at 3x speed (about 0.48s), then
return to rest. Queued throws retain the aim pose until they can fire. Rapid
shots do not restart the throw, and held input cannot replace an unfinished
throw with aim. Ongoing attacks,
air movement and Blade Mode retain their animation. Rapid taps are queued in
order (up to ten), each retaining its release aim and weak target handles.
A quick tap (less than six ticks) aims at a target in front of Raiden, or straight
ahead if none exists. Holding C for six ticks enables precision aiming, with the
reticle raised to 44% of screen height. The native camera unprojects that exact
screen position. Precision release uses the last held ray and converges at the
target's depth without snapping laterally toward its chest. Camera movement
after release does not steer knives; charged homing still follows enemies.
Throws are spaced six active frames apart: ten taps can fire in 54 frames (about
0.9s at 60 FPS), then a 120-frame cooldown applies. Volleys count each knife
toward the ten-shot limit; emptying a partial inventory also starts cooldown.
The cooldown bar does not replenish native ammunition. Hit-stop
freezes charging; death, story/QTEs, forced execution states, inventory changes
or loss of focus cancel a hold. After cancellation, release C before starting a
new charge. Normal grenades/RPGs retain native aiming and ammunition.
These interactions, reticle alignment and projectile behavior need in-game testing.

## Build

Requires Windows, Visual Studio 2022 C++ tools, Windows SDK and MGR Plugin SDK.
Set `MGR_PLUGIN_SDK` to the SDK root with a trailing backslash, then run:

```powershell
msbuild 'Raiden Moveset.vcxproj' /p:Configuration=Release /p:Platform=Win32 /m
```

Dear ImGui is included under `third_party/imgui`. Output is
`Release/Raiden Moveset.asi`. Build does not automatically overwrite the game.
Install with `tools/install.ps1 -GameDirectory 'path to game'` while the game is
closed. Restart the game after changing the installed ASI. Avoid loading a
separate weapon-switcher plugin alongside this integrated version.

## Validation and local files

Run `tools/test-all.cmd`. Tests require locally extracted, legally owned game
assets under `local_assets`; asset-backed tests fail if their fixtures are absent.
Prepare them with `python tools/prepare_test_assets.py 'path to game/GameData'`.
No game files are committed.
The current checks cover input/switch guards, DLC motion/sequence routing, effect
asset coverage, damage budgets and lifecycle policies. Native gameplay and
visual alignment require in-game testing; a successful build does not certify them.

Generated files, local assets, IDE state and backup/reference projects are ignored
by Git. Cleanup preserved previous source/reference files in `backups/cleanup-*`.
Licenses and source attribution are in [third_party/NOTICE.md](third_party/NOTICE.md).
Earlier runtime fixes are documented in [RUNTIME_REPAIR.md](RUNTIME_REPAIR.md).

### Encounter repair and Bladewolf secondary

Raiden's native D-pad inventory now includes **DLC3_BladeKnife** (Bladewolf
heatblades), subweapon slot 10, with ten knives on first registration. Select it
in the native inventory or use the mod's heatblade shortcut. Hold **C** or the
controller subweapon button to aim/charge, then release to throw. Successful throws
consume one knife through the native item class; failed/resource-blocked throws
consume none. The burst permits ten knives at six-frame intervals, followed by
a 120-frame cooldown. Selecting a secondary knife leaves
the equipped melee weapon intact. F7/F8 chooses the knife tap/volley payload.

Raiden ultimates load native `pl0010` motion/sequence pairs: thunder slice `2400`,
lightning storm `3501`, and lightning draw slash `2420 -> 2422`. Their hit IDs use
Raiden's original attack table, with native effect/audio timing retained; Sam's
boss sequences remain exclusive to boss add-ons. Ultimate action flags are
excluded because the addon controls its own duration and state transitions.

Story events and QTEs temporarily use native Raiden controls and then resume Sam.
The earlier encounter repairs retain scene effect/sound selectors, register
Sam boss/Bladewolf/projectile effects, and avoid global costume-table mutations.
See [RUNTIME_REPAIR.md](RUNTIME_REPAIR.md) for evidence and validation limits.
Native inventory rendering, save/reload behavior, and combat still need gameplay
verification. The plugin re-registers a missing knife item; it does not alter the
save-file format or replace another inventory item.
