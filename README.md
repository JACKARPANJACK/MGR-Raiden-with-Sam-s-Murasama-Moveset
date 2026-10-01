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
- **D-pad Left / Right**: previous / next weapon.
- **D-pad Up**: Murasama; **D-pad Down**: Raiden sword.
- **F7 / F8**: previous / next kunai variant; also selectable in the menu.
- **F** in Sam mode: sweeping finisher against a nearby weak cyborg.
- **X** or **controller B / Circle** in Sam mode: queue the next lightning ultimate.
- **Select / Back** or **L3 + R3**: toggle Sam's moveset.
- Directional Light/Heavy and flick inputs add boss attacks; see
  [directional controls](DIRECTIONAL_MOVES.md).

Weapons: Raiden sword, Murasama, Pole-arm, Sai, Pincer blades and Unarmed. Switching waits
for a safe ground state and waits for streamed weapon assets before equipping.
Weapon assets preload and stay mounted for the player's lifetime. Mouse-wheel
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
Selecting Murasama enables Sam's moveset; selecting another weapon disables it,
including a pending Sam activation. Q/E and the D-pad cycle through Murasama as
a separate choice. Secondary weapons and unarmed combat use Raiden's native
controller. G remains a synchronized Sam toggle. Switch inputs are reserved
during normal gameplay; scripted/QTE input stays native. Native finishers/Zandatsu use
their normal in-game prompts; F is a separate combat finisher.

Kunais are separate **subweapons**: Stun, Explosive and Heat-blade. All use grenade
ammo and Raiden's normal grenade aim/throw inputs and animations. Stun kunais
apply a short EMP stun to ordinary enemies; explosive kunais carry a native
grenade payload; heat blades apply bounded damage over time. Each uses Bladewolf's
physical `wp0372` projectile setup from `sub_16E2F0`. Selecting Native inventory
restores the previous grenade/RPG inventory selection. Choosing another item in
the game's inventory also exits kunai mode. During a kunai throw, Sam mode temporarily
uses Raiden's subweapon controller and restores Sam after aim/throw recovery.
These native interactions and particle visibility still need an in-game test.

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
