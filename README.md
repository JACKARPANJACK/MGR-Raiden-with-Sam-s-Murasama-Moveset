# Raiden with Sam's Murasama Moveset

The native inventory's custom-melee category includes **Unarmed**, **Dual Swords**, and
**Iaido katana**, alongside the original polearm, sai, pincer blades, and None.
Iaido katana equips the Murasama / native Sam moveset. Selecting another melee
entry restores that weapon's moveset. Inventory selections apply on returning to
gameplay, once switching is safe and the weapon resources are loaded.

Win32 ASI plugin for Metal Gear Rising: Revengeance. Selecting Murasama in the weapon selector enables the native Sam DLC
combat controller on Raiden, including charge/Iai, Blade Mode and Datsu nodes.
Non-2xxx DLC animations now route to Sam's motion and sequence assets, including
finishers and Zandatsu. Seven execution clips intentionally use shared native
motions with their Sam DLC sequences. Scripted story states retain native routing.

Full controls and move inputs: [Sam movelist](SAM_MOVELIST.md).

Full controls and move inputs: [Sam movelist](SAM_MOVELIST.md).

## Controls

- **Press G / R3**: cycle Standing → Crouch → Prone → Standing. Crouch and prone
  use retargeted MGSV idle/walk clips and slower ground movement.
  Stealth remains active while crouch walking and crawling: ordinary cyborg
  patrol perception ranges drop to 45% and 25%, respectively. Close proximity
  and existing combat alerts remain effective. Attacks, gunfire and sprinting
  expose the player for two seconds; bosses and other sensors retain native detection.
  **Jump while crouched:** dolphin dive toward the left-stick/WASD direction,
  or forward with no direction held. **Jump + left/right while prone:**
  tactical roll. Both finish prone so crawling can continue. Native collision
  stays enabled; low-obstacle clearance and cover attachment remain native.
  With Unarmed selected, neutral Light/Heavy keeps regular attacks.
  **Forward + Light** starts MGSV CQC; continuing Light during the fourth native
  punch appends its four-hit chain, making eight hits with continued Light
  presses. Imported hits chain or release control at recovery, without holding
  through their idle tails. **Forward + Heavy** grabs.
  During a grab, **direction + Light** throws, **Back + Heavy** uses the knife
  motion, and **neutral Heavy** releases. **F / controller B** attempts a rear
  stealth takedown. Prone CQC rises to a low grab. Ordinary jumps are blocked
  in crouch/prone; the dive/roll shortcuts remain available.
  CQC uses paired native MOT/BXM playback, fist collisions, attack/recovery flags
  and electric impact effects. These are experimental MGSV attacker clips with MGR victim reactions;
  paired victim clips and a visible knife prop remain unfinished. Low SMG
  aiming adds upper-body rotation offsets over crouch-walk/crawl loops, so
  movement continues while firing. RPG/grenades retain native aim/release arm
  rotations on the low body, with native trajectory/ammo/timing preserved.
  See [stealth animation port](MGSV_STEALTH_PORT.md).

- **Hold Ninja Run near a structure**: attach from the ground or air. Facing its
  wall runs upward; facing along it runs sideways. A clear roof permits a swept
  step onto the top. No native Ninja Run interaction marker is required.
- **Hold Z**: wall cling; release to drop. **Jump** kicks away.
  Controller cling uses **Ninja Run + L3**. **V** also requests wall traversal.
  See [wall traversal](WALL_TRAVERSAL.md) for current limits and diagnostics.

- **0**: open the menu, including the weapon selector.
- **Q / E** (also **[ / ]**): select previous / next weapon.
- **Mouse wheel Up / Down**: previous / next weapon, including Murasama and Unarmed.
- **D-pad**: native inventory, including the heatknife.
- **Inventory → Sword / primary weapons → Raiden R00 HF Blade**: equip
  Raiden's original prologue sword in normal Raiden gameplay.
- **F7 / F8**: previous / next secondary mode; also selectable in the menu.
- **C / controller subweapon button** with Bladewolf heatblades selected: throw.
- **Tap F (keyboard) / click LEFT stick, L3 (controller)** with Sai equipped:
  pull a living normal cyborg toward Raiden. Stand on the ground and face the
  target within 15 metres. The enemy is physically dragged to 1.2 metres from
  Raiden, including through the Sai throw/recovery animation. Hammer/heavy
  cyborgs and bosses are excluded. No cooldown: release and press to repeat.
  The weapon menu shows the last pull result. Diagnostics are saved to
  `scripts/SaiPullRuntime.log` with the player state and target ID.
- **Back + Heavy** with Sai equipped: Monsoon's launcher kick (Sai only),
  including his native kick effect. Sai stays equipped and visible. Hold S
  or left stick down and press the mapped Heavy attack while grounded and
  free to attack. The retargeted kick uses Raiden's native upward kick damage
  reaction and a physical upward impulse on a confirmed hit against a normal
  cyborg. Misses do not launch or add damage. Jump into an air combo after it
  hits; the weapon menu reports kick startup and confirmed launches.
  Light, Heavy, Jump and F / L3 pull can cancel the kick's recovery. Follow-up
  taps buffer for 12 updates, pause during hit-stop, and enter the native input
  selector from frame 44 after a confirmed launch or frame 64 after a miss.
  Holding the original Heavy press does not automatically repeat the kick.
- **Sai Y / Heavy**: Stabbing Despair (Combo List 35), with native Force of
  Magnetism (37) on the target follow-up. During throw recovery, press Y again
  for Double Throw Stab (36). After Force of Magnetism, continue Y, Y for
  Spin Slice (38) and Sommersault (39). In the air, Y performs Unavoidable Fate
  (40). Extra follow-ups buffer for 24 updates and start after frame 8; each
  step requires a fresh Heavy press.
- **F** in Sam mode: sweeping finisher against a nearby weak cyborg.
- **X** or **controller B / Circle** in Sam mode: queue the next lightning ultimate.
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
Changing weapon or subweapon slots does not force the sword onto its sheath.
Active aiming and low stances borrow the sword attachment for one rendered frame;
the original attachment returns before the next native update. R00 and regular
swords keep native draw, attack, Blade Mode and scripted visibility handling.
Gun poses transfer joint rotations onto the active costume's skeleton while
preserving its joint offsets and scales. Aim pivots stay at each upper arm;
costume-only deformation bones follow their posed parents. Run
`tools\test-gun-pose.cmd` for the pose and restoration regression checks.
Selecting Murasama enables Sam's moveset; selecting another melee weapon disables it,
including a pending Sam activation. Q/E cycles through Murasama as
a separate choice. Heatblades preserve Sam's moveset. Melee secondary weapons and unarmed combat use Raiden's native
controller. Sam's moveset is enabled only through Murasama weapon selection. Switch inputs are reserved
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

## Assault Rifle and KRISS Vector SMG

**M9 Tranq and SOCOM Mk23** are also separate secondary slots, with independent
15/12-round magazines and heavier recoil/knockback. Tap C/LB for one round, or
hold aim and press LMB/RT. SOCOM: hold C/LB for 0.5s or 1.5s, then release for
an electric shot. The local MGSV meshes are provisional handgun equivalents;
exact M9/Mk23 meshes were not found. See [pistol controls and asset limits](docs/PISTOLS.md).

The native secondary inventory, the **0** menu and **F7/F8** now offer two guns:

| Weapon | Model | Slot | Automatic cadence at 60 gameplay ticks/s | Handling |
| --- | --- | --- | --- | --- |
| Assault Rifle | Native `wp0040` rifle | 11 | One shot per 4 ticks (900 RPM) | Existing lighter recoil |
| KRISS Vector SMG | Native `wp0020` compact gun | 12 | One shot per 2 ticks (1800 RPM) | Faster bloom buildup, higher spread ceiling and added upward visual recoil |

Each gun retains its own 30-round magazine and 90-tick automatic reload. Switching
does not refill the previous magazine or transfer queued fire. Reload progress is
retained while the gun is stowed and resumes when selected. Knife ammunition stays
independent. Selecting another secondary cancels firing; the melee weapon stays
selected. Native inventory and mod-menu selections resolve to the same gun slots.

Hold **C / LB** to aim, then **LMB / RT** to fire. Aim alone does not shoot.
A quick C / LB tap releases a three-round burst at the lock target or nearest
enemy. Lock-on plus C / LB fires juggling rounds. Slides, parkour, story states,
menu opening, focus loss and death suppress shooting. Both guns retain the shared
rifle-cyborg body poses; the Vector mounts to the right-hand marker and emits from
its own muzzle bone `0x300`. The rifle keeps its left-hand grip motion and root
muzzle offset. Ordinary bullets deal 4 damage; launcher rounds deal 6.

The inventory has distinct names, descriptions and MGR-style icons for both guns.
Its three list arrays are relocated to 13 cells, retaining all ten stock entries,
both firearms and None. Item IDs 47 and 48 have separate native possession objects.
The selected gun's ammo appears in the menu; stock messages and icon pixels remain
preserved. English text and icon archives build into `Release/GameData`.

Run `tools/test-firearm-slots.cmd`, `tools/test-smg-controls.cmd`,
`tools/test-gun-pose.cmd`, and `python tests/weapon_ui_assets_tests.py`.
These check native instruction relocation, full inventory capacity, gun selection,
fire cadence, magazines/reload, both weapon grips, restoration and asset integrity.
Final in-game appearance, weapon switching and combat still need a gameplay check.

### RPG and Stinger aerial / charged shots

Select the native RPG or Stinger (LAG) in the game's secondary menu. Tap and
release **C / controller subweapon** to fire on the ground or in the air.
Hold then release after 0.5 / 1.5 seconds for 2x / 3x native attack damage.
Each successful shot consumes one native rocket; empty inventory or failed
resource/projectile creation consumes nothing. The native models, RPG `wp1002`
and guided Stinger `wp10E1` descriptors, battle parameters and BulletBase factory
are retained. Stinger uses a live native lock target, or the nearest forward
enemy when no lock is available. Projectile speed remains native (200).
Hit-stop pauses charge and cooldown; weapon changes, death, scripted states,
Blade Mode, opening the mod menu and focus loss cancel pending shots. Camera
aim UI and ground aiming sequences are bypassed for this C firing path; these
new aerial shots and model alignment need in-game verification.

## Build

Requires Windows, Visual Studio 2022 C++ tools, Windows SDK and MGR Plugin SDK.
Set `MGR_PLUGIN_SDK` to the SDK root with a trailing backslash, then run:

```powershell
msbuild 'Raiden Moveset.vcxproj' /p:Configuration=Release /p:Platform=Win32 /m
```

Dear ImGui is included under `third_party/imgui`. Output is
`Release/Raiden Moveset.asi`. Build does not automatically overwrite the game.
Install with `tools/install.ps1 -GameDirectory 'path to game'` while the game is
closed. The installer also requires the paired animation bundle in
`Release/SamTemplateAnims`; build it from the local campaign template using
`tools/build_sam_template_executions.py` as described in
[directional controls](DIRECTIONAL_MOVES.md).
Restart the game after changing the installed ASI. Avoid loading a
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

The light-combo continuation uses the campaign template's `pl1400_9000` through
`9003`. Build its portable bundle before installation:
`python tools/build_sam_light_extensions.py --template "E:/Sam in Raiden Campaign/mods/PlayerEnemy/pl/pl1400.dat"`.
In Sam mode, keep pressing neutral Light through the base finisher to queue the
extension; each subsequent Light press buffers its next stage.
