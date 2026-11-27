# Raiden pistols

## Directional pistol combos

With M9 or SOCOM selected, hold a left-stick direction and press LB
(keyboard W/S/A/D + C). The complete route plays once per press:

| Direction | Attack route |
| --- | --- |
| Forward / W | Three standard pistol attacks |
| Back / S | Two downward pistol attacks |
| Left / A | Standard, downward, standard |
| Right / D | Downward, standard, downward |

**Regular shooting takes priority:** C+LMB / LB+RT uses the normal semi-auto
pistol controls, including while moving. Triggering regular fire during
gun-fu cancels the buffered chain and hands off one shot to normal aiming.
Holding the trigger cannot repeat shots or add a release shot; SOCOM charge
is retained for ordinary aim/fire.

During gun-fu, another directional C/LB press or a change/flick of the held
direction buffers one follow-up attack. Up to four pending attacks are kept
in order, including the remaining attacks of the starting route. Overflow is
ignored without replacing older commands. Follow-ups start after the current
volley finishes; they do not restart the current attack or pass through idle.
Four animation frames of parent-space pose blending join consecutive attacks
while preserving limb lengths. Jump, melee, sprint, weapon changes and
gameplay cancellation clear the pending chain.

These are full-body native MOT/sequence pairs retargeted from the supplied
InsanePistolAction generic in-place FBXs, baked at 60 Hz with Raiden's bone
lengths and no actor root travel. Both attacks live in `wp2062.dat` at
`2f50`/`2f51`; existing poses and grip bytes are preserved. Each clip requests
three ordinary pistol rounds across its firing sweep through the existing
native bullet pool; ammo, reload, M9 sedation and SOCOM contact effects remain
owned by the pistol runtime. A complete forward/left/right route fires nine
rounds; the back route fires six, subject to available ammunition. An empty magazine can skip a combo shot while
reloading. These routes do not create melee hitboxes or paired victim actions.

Attack-only retarget: source frames 14â€“50 are baked into 37-frame clips.
Equip/raise prefixes and lowering/idle tails are excluded. Skin twist helpers
receive axial twist only; elbow/wrist flexion stays on the main joints. Limb
translations stay native to the active costume. The hands use the existing
pistol grip with a two-arm reach solve, and the feet retain the native floor.
All six actor-root channels explicitly contain zero motion. Exported MOTs are
independently reimported and rendered with the pistol on installed body meshes.

Each clip fires a three-round volley at frames 2, 18 and 34, then yields at
frame 36. Volley cadence follows animation time so low rendering frame rates
do not drop scheduled rounds. Shots originate at the current animated muzzle and travel along
its forward axis as the wrists, torso and player heading move. Combo shots
follow that authored direction, including downward pitch; normal held aim
retains the camera and lock-on aiming behavior. The old 99-frame tail no
longer holds movement through every combo step. Playback reads the actual
animation node's remaining time. Eight active ticks without progress abort
the route instead of starting another step. Completion/cancellation clears
the owned attack-lock flag and installs native idle, including its sequence.
Custom action playback supplies the MOT/sequence with `setDirectAnimation`
and runs native common animation/weapon maintenance (`0x794790`) every active
update. Skipping that maintenance left node zero at frame zero; executable
checks now cover its animation-slot update tail and direct playback resets.

Full-body playback owns the final visible pelvis, torso, head, arms, hands,
legs and feet. Native MOTs explicitly author all local channels. Matching
`FBW1` tracks apply after stance/traversal/melee layers and before mounting the
pistol and emitting rounds. The final pose fits the captured costume offsets
and carries additional costume bones through their native parents. Every
temporary matrix/flag restores before native update. Native BXM lock/recovery
spans are contiguous, nonempty and bounded by the matching MOT duration.
`tools/test-pistol-full-body.cmd` exercises both clips at three costume sizes,
including competing poses, actor transforms and exact apply/restore behavior.

Combos start from grounded locomotion. Jump/heavy attack/sprint, airborne transitions,
weapon changes and gameplay interruptions release ownership; pause and hit-stop
freeze playback. WASD/stick + C/LB enters gun-fu directly, bypassing aim poses,
crosshair and charge when LMB/RT is not held. Adding a direction while C/LB is
held also hands off to gun-fu until regular fire has been used in that hold.
Diagonals use the dominant axis, with equal diagonals choosing
forward/back. The press stays consumed through combo completion or
cancellation, so holding C/LB or centering the stick cannot enter aim afterward;
pressing LMB/RT explicitly returns to regular shooting.
Release C/LB to rearm. A fresh neutral C/LB press retains single shots, held aim
and SOCOM charge. Input directions select the route; they do not rotate the camera.

Rebuild attacks with Blender running `tools/build_pistol_attacks_blender.py`
with `--repo` and `--package` after `--`. `tools/build-pistols.cmd` automatically
includes the package at its supplied Desktop location, or the path in
`PISTOL_ATTACK_PACKAGE`. Check with `tools/test-pistol-combos.cmd`.
Visual gameplay verification is still required.

M9 Tranq and SOCOM Mk23 are separate secondary weapons (slots 13 and 14).
Select them in the native inventory, the 0 menu, or with F7/F8. The rifle,
Vector and knives retain their existing slots and controls.

| Weapon | Magazine | Minimum shot interval | Payload |
| --- | --- | --- | --- |
| M9 Tranq | 15 | 18 active ticks | Zero HP damage; five seconds of bounded tranquilizer slowdown on ordinary cyborgs |
| SOCOM Mk23 | 12 | 15 active ticks | 12 damage; electric charges deal 18 or 30 |

Tap C/LB to fire one round. Hold C/LB to aim and press LMB/RT for one shot.
The pistols are semi-automatic: release the fire trigger between shots.
SOCOM charges while C/LB is held. Release after 30 active ticks (0.5 seconds)
for an electric shot, or 90 ticks (1.5 seconds) for a full electric shot.
Firing with LMB/RT during a hold consumes that hold's release, preventing a
second accidental round. The reticle shows charge progress and payload.

Each weapon retains its magazine and reload progress when stowed. Empty
magazines reload in 90 active ticks. Switching, menus, death, loss of focus,
melee, slides and traversal cancel pending fire/charge; release C/LB to rearm.
Hit-stop freezes charge and reload. Failed projectile spawns consume no ammo.
Pistols have stronger visual recoil than the Vector. Confirmed ordinary-cyborg
hits apply horizontal knockback; electric hits add native lightning and bounded
slowdown. Bosses are excluded from these custom hit reactions. M9 slowdown is
a mod tranquilizer approximation, not MGSV's sleep/victim animation system.

## Assets and limits

The local MGSV search found `hg01_main0_def.fmdl` and `hg00_main0_def.fmdl`.
These are provisional MGSV handgun donor meshes, **not exact Beretta M9 or HK
Mk23 models**. No exact replacement meshes were found in the searched local
repositories or the MGR installation. Native wp0020 material/textures are reused;
Fox textures, animated slides, magazine props and a handgun reload clip are
not ported. No USP entry is added.

Five MGSV clips are evaluated through the existing Fox Rigify IK pipeline and
baked to Raiden: standing ready, raise, lower, crouched ready and prone ready.
Runtime uses the appropriate ready pose over Raiden's native movement. Raise
and lower clips are packaged for further transitions. The gun binds to right
wrist 13, with a grip basis derived from the neutral standing source pose. Bullets and
muzzle effects use the same rendered barrel transform. The native skeleton,
joint lengths, costume helpers and movement are restored between visual frames.

Packages: `Release/GameData/wp/wp2060.dat` and `.dtt`, `wp2061.dat` and `.dtt`,
and `wp2062.dat`. Both DAT and DTT bytes are retained for the process
lifetime and bound together through `Hw::cFmerge::setData(dat, dtt)`. Custom model
factories share the existing wp2050 hooks. Each pistol uses its own model and
behavior resource ID; the native virtual model loader prepares the WMB, GPU
buffers and texture pair. Missing/invalid pistol packages fail closed and
report status in the 0 menu. No retail CPK is modified.

Build with `tools\build-pistols.cmd` (or `tools\build.cmd`). These scripts read
the locally installed MGSV files. Generated proprietary assets stay in ignored
local/Release folders. Install with:

```powershell
./tools/install.ps1 -GameDirectory 'E:/SteamLibrary/steamapps/common/METAL GEAR RISING REVENGEANCE'
```

Checks: `tools\test-pistols.cmd`, `tools\test-firearm-slots.cmd`,
`tools\test-smg-controls.cmd`, `tools\test-gun-pose.cmd`,
`python tools/verify_pistol_assets.py`, and the native executable/UI checks.
The pose regression covers 720 frames, including all three handgun stances,
different costume proportions, grip placement and exact pose restoration.
The exported WMBs are reimported independently into Blender with their actual
WTA/WTP and checked for render geometry, shader and rigid root weights. The
retail executable's Fmerge lookup is exercised against both DAT/DTT pairs.
Zero-damage hit callbacks, tranquilizer behavior and electric reactions still
require gameplay verification.

## Invisible-model repair

The original runtime reported status 5 (entity creation failed) for both
pistols. It supplied raw WMB bytes through `EntityInfo::m_pModelData`, which
actually expects the engine's prepared `EntryModelData*`, and supplied only
the DAT half of each archive pair. The repaired path leaves the override null,
uses each pistol's own resource identity and supplies DAT plus DTT to the normal
native loader. Creation now also checks that a render mesh and root bone exist.

The Blender exporter additionally replaced texture flags 0x0e/0x0f with 0x63.
The build now restores these flags by texture hash from the native material
donor. Verification checks that every local-texture ID resolves to an in-bounds
DDS payload in WTP, while resident texture flags remain intact. Live M9 logs
after the repair confirm one render mesh, successful attachment and bullet
emission. SOCOM uses the same corrected path with its separate DAT/DTT pair.

The handgun pose grids now use neutral frame 12 standing/crouched and 13 prone;
frame 0 is the upward corner that previously raised both hands overhead.
Runtime analytic two-bone IK preserves each costume's upper-arm and forearm
lengths, uses the authored elbow bend planes, and anchors the support hand to
the solved firing hand. The enlarged model's grip center is fitted to the active
costume's wrist/finger palm landmarks after IK. Both meshes are 1.25x their MGSV
size; muzzle offsets use the same scale. Neutral-frame posture, 429 IK targets,
palm fit and all three stances are covered by the asset/runtime regressions.

Sword docking now refreshes the player-to-sheath attachment and sheath hierarchy after body overlays, before sampling the sword socket. Temporary docking supports both contiguous bones and pointer tables and restores native matrices/flags before the next tick. Native sword draw state and constraint ownership are preserved.


Low-stance gun composition: apply crouch/crawl first for all four firearms; pistols then overlay only arms/fingers and solve IK, retaining the low torso and head. The stance layer is marked applied so the later post-tick pass cannot overwrite the solved grip. Low-body rotations are fitted to native costume offsets/scales, including extra jacket/twist/attachment bones, and all temporary matrices restore before native update.


M9 sedation: confirmed native pooled contacts accumulate 1/2/3 doses for targets with max HP up to 200/up to 500/above 500. Doses expire after 10 seconds. Sleep enters the native cyborg A0004 knockdown through the retail state setter. The native 5300/5303 fall finishes before phase 3 is held on the floor for 30 seconds using an actor-local zero-rate unit. HP damage, death, teardown, an action change or a replaced slow controller releases the hold; wake-up selects native get-up phase 4 only while we still own the floor state. No manual bone or skinning transforms are applied. Boss controllers remain native.
Charged SOCOM enemy contacts instantiate wp1051 with native EMP attack 0x58 and effect/sound IDs 0x51/0x11. The payload fuse is one tick, and no grenade inventory is consumed. Current trigger is confirmed enemy contact; world-surface impacts are not hooked.


`wp2062.dat` is the shared pistol animation bank, using the native `wp` plus
four hexadecimal digits archive naming convention (weapon resource ID `0x32062`).
It is read directly from `mods/RaidenMoveset/wp`; it does not create a third
pistol weapon. Its custom members retain their existing DAT lookup hashes.

Humanoid gun-fu attacks: the shared bank now contains all eight player-attack
families from the supplied InsanePistolAction package, with both source variants
(16 native MOT/BXM/full-body pairs). Each FBX resets to frame zero before bind
calibration, and its bind geometry is checked against the Humanoid T-pose.
Free attack hands retain their authored motion; aerial movement and prone
pelvis height are preserved. Paired victim and contact gestures stay outside
the player routes. See [the complete FBX audit](PISTOL_FBX_COVERAGE.md).

Direction + C/LB selects a buffered gun-fu route. All attack variants are
reachable as the route cycles advance; C+LMB / LB+RT retains normal shooting
priority. Projectile volleys follow each final animated muzzle pose. The
pistol-throw body motion uses the held gun and projectile system, without a
physical thrown-gun prop. Newly added moves still require in-game evaluation.

Rebuild: run `tools/catalog_pistol_package.py --package PACKAGE`, then Blender
with `tools/build_pistol_attacks_blender.py -- --repo REPO --package PACKAGE`.
Use Blender `--background --factory-startup --python-exit-code 1 --python SCRIPT`.
`--only FAMILY ...` rebuilds selected families without changing other clips.
Run `python tools/tune_pistol_combat.py` after authoring to produce the compact
combat windows from an archived full-length bank. It is safe to rerun.
Run `tools/verify_pistol_sources_blender.py` through Blender for an independent
reverse-order source comparison; then `tools/test-pistol-combos.cmd`.

Install the plugin only as `Scripts/Raiden Moveset.asi`. A second copy in the
game root installs competing controllers: one starts a gun-fu move and the
other can abort it immediately with an unloaded playback clock. The installer
now verifies the current Scripts copy, backs up any root duplicate, and renames
that duplicate to a disabled extension. Full-body retarget data is unaffected.

The combo controller now requires its own running state, loaded pairs and a
prepared body pose before handling action 0x143. Gun rendering uses the same
ownership callback. Native interruptions release the temporary pose without
replacing the incoming native action. The integration regression loads all 16
actual exported pairs and checks three-stage playback, all nine volley cues,
manual-fire handoff, jump cancellation, foreign actions, stalled playback and
weapon-switch cleanup (`tools/test-pistol-controller.cmd`). Runtime build v23
logs the clip code, expected duration and measured time for playback failures.


Responsive gun-fu profile (v23): direction + C/LB starts one attack per command.
Tap C/LB again while holding a direction, or change direction while holding
C/LB, to request a follow-up. One pending input is retained for 0.4 seconds;
repeat taps refresh it and a different direction replaces it. The next attack
starts after the outgoing move finishes its volley, cutting recovery. Pausing
for 0.9 seconds after an attack resets the route to its opener.

The 16 Humanoid attacks retain their source bone poses. Longer routines use
compact attack-only windows and 1.18–1.35x playback rates. Entry and follow-up
poses blend in parent space with smooth easing, preserving limb lengths at
costume scale. The native MOT, BXM and full-body tracks share the same trimmed
frame range. Volleys use source-time cues and the visible muzzle transform;
low-frame-rate crossings retain the final cue. C+LMB / LB+RT immediately returns
to regular shooting. Jump, melee and ninja run cancel the gun-fu action.

The responsive profile has automated controller, source-continuity, native
MOT/full-body agreement and costume/blend validation plus independent rendered
pose review. Its final combat feel still needs a fresh in-game session.


Temporary decoupling: the base ASI now defaults `MGR_ENABLE_PISTOL_GUNFU` to 0
in `PistolGunFuFeature.h`. Its controller/action hooks, animation requests and
pose callbacks are excluded from the base build. Directional input no longer
captures C/LB for gun-fu; ordinary pistol aiming and firing remain available.
The retarget source, animation bank and controller are retained for later work.
