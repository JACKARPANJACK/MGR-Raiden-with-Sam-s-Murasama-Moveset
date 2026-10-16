# Rifle-cyborg gun set

The SMG now follows the rifle pose's left-wrist firing grip (bone 9), using
the rifle's grip rotation and translation instead of the enemy right-hand
marker 0x701/13. Its native muzzle marker 0x300 remains unchanged. Both the
native constraint and current-frame visual aim use this grip. The 720-frame
pose regression passes; in-game visual verification remains pending.

The firearm inventory now has separate Assault Rifle (slot 11, wp0040) and
KRISS Vector SMG (slot 12, wp0020) entries. The rifle retains its four-tick
fire interval. The Vector fires every two ticks, builds bloom by .035 rather
than .015 per shot, caps bloom at .16 rather than .08, and adds a .14-radian
visual recoil kick. Both retain separate 30-round magazines and reload state.
The Vector uses native right-hand marker 0x701 and muzzle 0x300; the rifle keeps
its left wrist 9 grip MOT. The shared body animation donor remains em0010.
See the current controls in README.md.


The donor is the basic rifle cyborg `em0010`, whose shared motion bank is
`em0012.dat`. Native sequence gun-handling and magazine events identify its
rifle clips. `em0040` is a tripod rig and is no longer used for player poses.

The generated local package is `Release/GunMoveset/pl/pl0010.dat`. It preserves
all 1,236 members of the current loose Pl0010 archive and adds 34 retargeted cyborg motions:

| Pl0010 code | Cyborg source (`em0012`) | Role |
| --- | --- | --- |
| 2e00 | 2400 | Lowered ready stance |
| 2e01 | 2401 | Gun preparation/raise |
| 2e02 | 2404 | Upward aim; not used as firing recoil |
| 2e03 | 2402 | Level firing stance and selected idle |
| 2e04 | 2403 | Lower rifle |
| 2e05 | 2405 | Downward aim |
| 2e06 | 2406 | Magazine reload |
| 2e10 | 2410 | Walk start / advance |
| 2e11 | 2420 | Walk forward loop |
| 2e12 | 2428 | Walk stop |
| 2e13 | 2429 | Walk stop (alt) |
| 2e15 | 2425 | Walk backward |
| 2e16 | 2426 | Walk left strafe |
| 2e17 | 2427 | Walk right strafe |
| 2e20 | 2419 | Run forward loop |
| 2e21 | 2431 | Jog forward loop |
| 2e22 | 242a | Run stop |
| 2e23 | 242b | Run stop (alt) |

Retargeting preserves compressed rotation curves and matching body bone IDs,
adapts translations relative to Raiden's bind lengths while retaining pelvis
height changes and walk bob, removes actor root travel,
and drops the absent enemy weapon-control bone 0x630. Enemy sequences are not
copied. This is a skeletal adaptation, not an IK hand-alignment bake.

Secondary inventory slot 11 owns the gun. When equipped, player locomotion requests
(idle, walk start/loop/stop, and run start/loop/stop) are intercepted via
`Custom_Behavior_RequestAnimationByName` and redirected to `2e00` (lowered idle) and the `2e40`–`2e4f` locomotion variants, giving Raiden natural rifle soldier movement without freezing or locking up.
Firing while moving retains the cyborg locomotion clip; the torso aim correction
keeps both hands together without restarting the legs.
Melee attacks and aerial actions retain their native motion or transition gracefully. Deselection or gameplay cancellation releases
the gun pose and model. Missing packaged motions block gun readiness.

Build locally with:

```powershell
python tools/build_gun_animations.py --base 'path/to/current/pl0010.dat' --skeleton 'path/to/Raiden.wmb'
tools/build.cmd
```

Both the new ASI and generated Pl0010 archive are required. Install the archive
through the active mod profile, retaining a backup of the previous archive.
The ASI and both active Pl0010 packages have been installed with backups. Gameplay has not been verified. In-game verification must
check grip/muzzle alignment, transitions, reload, and inventory cancellation.

## Rifle effects and bullet descriptors

Gun geometry comes from wp0040; its companion wp004f supplies wp0040.eff
and wp0040.bnk. The effect archive registers under its streamed wp004f identity; muzzle effects use that same bank ID.
The gun uses native muzzle effect 0 and the rifle's root-space firing offset
(0, 0.01, 0.39), rather than the previous SMG muzzle marker 0x300.

The rifle cyborg calls a pooled bullet service with a native wpb000 descriptor.
The player controller uses that descriptor's attack 0x146, collision selector
0x19 at descriptor offset 0x110, 100-unit speed, 0.8 trajectory scale and 0.998 drag through
the native `BehaviorBalkan` pooled bullet emitter. Its `etb000` geometry is streamed
alongside the gun; the bare `wpb000` descriptor asset has no rendered bullet model.
Ammo is committed only after a pool slot activates with the player owner handle.
Player damage is 4 and player ownership/friendly filtering are retained. No enemy AI object
is passed into the player controller. Native executable checks and policy
tests verify the descriptor constants; gameplay remains untested.

The rifle's separate native owner handle at `cEm0010Weapon + 0x8CC` is populated
alongside the hand constraint. Native rifle updates treat an empty handle as a
detached weapon. Constraint ownership is checked before readiness. During active
aiming, the sword can be parked visibly on its sheath for that rendered frame.
Its original hand/foot attachment and draw state return before the next native
update. The gun does not change the sword entity's display flags.

Effect registration failures do not block model attachment or projectile creation.
The wp004f archive itself is required because it supplies the native grip motion.
`scripts/GunRuntime.log` records selection, eligibility, asset loading, model
attachment, projectile creation and caught native exceptions. Bullet/muzzle failure
statuses persist instead of being replaced by Ready on the next frame.

Tap C releases a three-round burst. Taps during an active burst add three queued
rounds without restarting a preparation delay, allowing repeated taps to sustain
fire. Holding C for at least 30 frames (0.5 seconds) charges; release requests
exactly 30 rounds and reloads a partial magazine first. Aim prioritizes a live
locked enemy, otherwise the nearest live enemy within 35 game units, including
enemies behind Raiden. With no target, the rifle aims and fires forward.
With no lock and multiple enemies in range, each round samples the group area
for suppressive fire while the stance remains aimed steadily at the nearest
enemy. A live lock disables the spread and remains precise. When Raiden is above
the target, the aim point drops to the enemy's lower body. Melee, jumping and
scripted actions release the gun pose and return sword handling to native animation. Grounded
walk/run requests use the cyborg locomotion bank.

## Native grip and aim correction (runtime 9)

The native rifle equip path at RVA `73A3FF..73A438` attaches to **left wrist 9**
and selects `wp0040_0001.mot`. Its root translation is (-0.097, -0.028, 0.077)
and rotation is (90 degrees, 0, -90 degrees). The runtime plays that native
weapon motion with zero extra constraint offsets, applying the grip exactly once.
`wp0040_0000` belongs to the holstered/back attachment and must not be reused.
The right hand is the support hand in the source rifle pose.

`2401` raises from lowered ready into `2402`, the level firing stance. `2404`
is upward aim, which caused the overhead pose when used as continuous recoil.
Idle and firing now use `2402`. Shots wait for the raise/reload animation and do
not restart the stance on every bullet. The animation request context includes
the gun so native idle/walk/run replacements actually reach the motion lookup.

Aim rotates chest bone 3 and its descendants together, retaining the donor's
wrist orientations and elbow planes. Native world-to-parent rotation conversion
and full hierarchy/constraint refresh are applied before muzzle sampling.
Three correction passes account for muzzle movement around the chest pivot.
The correction is bounded to 60 degrees per pose; stationary Raiden turns his
body to the target. Original chest rotation/quaternion/flags are restored before
the next animation update and on cancellation. No arbitrary wrist displacement
or independent hand IK is applied. Raise and reload retain authored hand motion.
Bullets follow the rendered muzzle direction; group suppression is enabled only
when the aim solve reaches its target.

Validation: release build, native executable checks, existing gun policy tests,
C++ aim geometry tests (yaw/pitch, muzzle offset, grip distance, turn limits,
invalid targets), and 1,557 sampled frames across all 18 generated MOTs. The
animation checks verify retained rotation curves, grip orientation/spacing,
raise/reload endpoints, and walk bob. Every unrelated member of both installed
Pl0010 archives is compared before installation. Both archives and the plugin
are backed up and hash verified. These checks do not replace an in-game visual
check; this revision has not been gameplay verified.

## C-triggered summon (runtime 10)

Gun selection retains cyborg idle/walk/run routing. Idle uses lowered ready
2400 (`2e00`); the level 2402 stance is reserved for the summoned firing phase.
The attached rifle stays hidden until C (or the corresponding gamepad input)
is pressed. It remains visible during charge, the released burst, and a queued
magazine dump/reload, then hides as soon as no input or rounds remain. Passive
magazine refill does not summon it. Idle selection applies no target-facing
rotation or torso aim correction. The corrected native grip stays unchanged.

## Free movement repair (runtime 11)

The old generator zeroed actor root travel on walk and run clips, causing legs
to animate in place and the stationary gun controller to keep claiming the player.
Sixteen new `2e40`–`2e4f` locomotion variants now sample the Em0010 body poses
onto each native player movement clip's duration and preserve its complete actor
root channels. The native player movement state and sequence retain ownership
of input, travel, heading and transitions. Enemy scripts are never executed.
Each player request has its own matching variant, including separate walk/run
starts, loops, stops and combat locomotion. Unsupported turn requests remain native.

Stationary detection includes keyboard movement, stick input and the player's
input magnitude, so raising, aiming or reloading cannot take over while movement
is requested. Moving also replaces an owned direct stance node immediately.
C-triggered visibility remains: the gun is hidden between firing requests.
Validation includes 930 locomotion frames compared against native root curves
and enemy body samples, input-before-speed tests, and the existing pose/aim tests.
Live gameplay of runtime 11 is still unverified.


## Native movement with visual cyborg poses (runtime 12)

Gun selection no longer replaces player animation requests or starts a player
Direct animation node. The player state graph, sequences, root travel, heading,
walking and ninja-run controls continue unchanged. After the native update,
rotation-only cyborg curves are applied visually: enemy locomotion on the legs,
with independent raise, firing and reload curves on the upper body. Every visual
rotation is restored before the next native update. Pelvis/root translation and
movement speed are never written by the gun controller. Firing input reservation
also excludes movement, sprint, jump and melee bits from composite bindings.

C still summons the hidden rifle for firing. Without C or buffered shots, the
selected gun uses relaxed cyborg idle/walk poses without target-facing correction.
Running and shooting are allowed simultaneously. Visual curves live in 34 `.gvp`
archive members, sampled from the existing retargeted Em0010 motions. Validation
checks all 2,487 frames for matching rotation values and absence of root/translation
channels, along with the gun policy and aim geometry tests. Live gameplay has not
been verified for this revision.


## Barrel flash, spray and aerial combos (runtime 13)

Muzzle flash uses the verified 68C930 world-position/rotation emitter. Its position
is the same barrel-tip origin used by each native bullet, rather than the gun
entity root. Effect direction follows the barrel independently of spray direction.

Tap C for the normal three-round burst. Shift+C requests wide spread; releasing
a charged C request dumps the magazine with spread. An unlocked group is covered
by suppression sampling, while locked targets and empty areas use a barrel-relative
cone (approximately 33 degrees wide and 11 degrees high). Shift retains native
sprint input. These modes are also available while airborne.

Firing no longer cancels on jumps, melee combos or slides. Unsupported movement
poses keep their native legs/root, while gun arms animate independently. Air/slide
shots skip the stationary raise delay. Airborne enemies are preferred when Raiden
is airborne unless native lock-on supplies a target. Confirmed, attributed bullet
hits on ordinary airborne enemies refresh a modest upward speed with a four-frame
per-enemy cooldown. These hits also cap Raiden's downward speed during air combos;
misses and ground hits provide no aerial hang. Bosses, dead enemies, story events
and paused frames do not receive this juggling adjustment.

Validation: gun policy/buffering tests, rotated/pitched spray geometry, muzzle
position checks, native flash ABI checks against the local game executable, and
existing animation tests. This revision still requires an in-game visual check.


## Complete grip pose and bounded aim (runtime 14)

Visual pose archives now supply all six local channels for every positive bone:
missing rotation tracks use zero and missing translation tracks use Raiden's bind
lengths. Upper-body locals include these translations so native sword shoulder,
wrist and finger values cannot leak into the rifle pose. Root/pelvis and all lower
body translations remain native. Every applied local is restored before the next
native update. Only known leg chains receive locomotion rotations.

Upper-body clip changes blend for six frames along the shortest angular path.
Torso correction is limited to 45 degrees on ground and 75 degrees in air/slides.
The pose checks validate all 2,487 complete local frames and both wrist-to-chest
transforms against the authored rifle grip. Build and gun tests pass; this version
still requires an in-game visual check.


## Native cache flags and quaternion aiming (runtime 15)

Native 616680 skips Euler-to-quaternion rebuilding on flags 4002 and world-matrix
rebuilding on flags 8004. Earlier pose versions preserved these flags, so correct
sampled locals could leave stale native sword matrices visible. Applied pose bones
now release both gates for their visual frame. Shared-model players refresh the
same parent bone-storage owner used by native getPartsPtr; 617A40 skips the child.
The original matrices, world scales, locals, quaternions and flags are restored
before native movement/animation update.

Aim correction decomposes the desired world orientation into a parent-relative
quaternion directly. Flag 4000 keeps this quaternion authoritative during native
hierarchy propagation; the Euler decomposition loop is removed. Existing turn
limits remain. The one-shot muzzle ESP uses an unbound world-space origin rather
than an entity attachment that can relocate authored units to the gun root.

Regression checks cover all 65,536 flag combinations, parent-relative quaternion
reconstruction at vertical/upside-down orientations, native EXE cache-gate bytes,
and existing grip, muzzle, spray and animation frames. Release build passes.
Gameplay and effect placement still require an in-game check; no claim of a
watertight visual result is made without that check.


## Authored world-pose application (runtime 16)

Rifle poses no longer run when the gun is hidden. Native movement, sword idle,
combat, jumps and slides retain their normal animation between summons.

During a summon, complete cyborg FK matrices from retargeted motions are sampled
from 34 new .gwm members. They are placed at the native pelvis position using the
actor basis and marked world-fixed for this visual frame. Local Euler channels,
physics, player animation nodes, native root travel and movement input are never
modified by this layer. Original matrices and flags are restored before the next
native tick. Idle/walk/run legs use the authored locomotion matrices; unsupported
movement and air/slide legs remain native. Aim correction rigidly rotates the
whole authored chest/head/arm chain, so both hands retain their relative grip
without independent wrist corrections or Euler decomposition. Aim limits are
30 degrees on ground and 45 degrees for air/slide poses.

Validation checks all 2,487 matrix frames and both wrist-to-chest chains against
source FK, orthonormal matrix interpolation, existing pose/grip/spray tests and a
release build. Both plugin and archive files must be installed together. Live
visual verification remains necessary; these checks alone cannot certify that
all in-game graphical issues are gone.


## Camera and lock-on aiming (runtime 17)

Unlocked gun aim unprojects the viewport center through the native camera. Its
convergence plane is 35 metres beyond Raiden, accounting for camera distance.
Native camera lock-on uses the selected living enemy and its native lock point.
Nearest-enemy targeting and incidental sword target handles no longer redirect
gun aim. The point is refreshed after the native camera/player update. A center
reticle appears during unlocked summons; amber indicates an unsolved pose.

Visual yaw turns the body for rear targets, limiting waist twist to 35 degrees.
Air/slide legs retain their native pose under that visual turn. World matrices
and flags restore before the next native update; movement input, physics and
actor rotation remain native. The rigid chest/hand chain handles remaining aim
correction with a 65-degree budget. Bullets and spray follow the rendered barrel;
spray no longer redirects randomly toward a group behind the camera.

The behavior ESP constructor inherits a slot-FF entity binding and can install
a model override. Appending a null slot-0 binding left those in place. Muzzle
flash now uses the standalone native ESP initializer and direct world-space
emitter, with the same final barrel origin used for projectiles. Native EXE
checks cover the constructor binding and world-space argument ABI.

Release build, gun policy/aim geometry tests, all 2,487 animation matrix frames,
and native ABI verification pass. Live gameplay remains unverified.


## Stable hold-to-fire controller (runtime 18; supersedes earlier input rules)

C now fires on press and sustains one round every four gameplay ticks. Release
stops immediately and hides the gun. Reload resumes held fire automatically.
No raise, reload or charge-release animation blocks an otherwise valid shot.
Shooting overlays ordinary attacks, movement, air and slides; story/forced
sequences, menus and death retain the existing cancellation guard.

The body uses a persistent, bounded world heading. Gun pitch affects both arm
chains rigidly around the shoulder line, not the chest/head. The head remains
upright; both hands retain their relative grip. The ready sample stays stable
instead of restarting donor scan/raise/reload clips. Ground movement selects
cyborg forward/back/strafe loops and blends clip changes over eight frames.
Native locomotion graph requests no longer reset the gun stride phase.

All native bone matrices are captured before either body half is modified and
restored before native update. The weapon render matrix is computed directly
from the current wrist and native wp0040_0001 grip, so the gun, flash and shots
cannot depend on the previous weapon update's matrix. Runtime tests exercise
720 frames of turning, pitching, moving and clip transitions with the actual
packaged poses, checking head/chest stability, rigid grip and exact restoration.

Only GameData/pl/pl0010.dat remains active. Both old archives are backed up;
all 1,338 existing members are preserved. The installer merges by member name
and prevents future duplicate deployment. Live gameplay remains unverified.


## Running & gunning, strict camera-view targeting, spray sweeping, and active lock-on tracking (runtime 19)

### 1. Running and Gunning Locomotion
- **Pelvis & Leg Free Movement:** When moving (`!stationary`), `NativeGunPose::Apply` no longer forces world-fixed pelvis transforms (bone 0 `flags |= 4`), preventing leg locking and sliding. Lower body locomotion plays natively or via retargeted cyborg running motions, while the upper body maintains rifle aiming and firing.
- **Locomotion Animation Routing:** `Custom_Behavior_RequestAnimationByName` sets `request.owner = PlayerAnimationRequest::Owner::Gun;` so player locomotion requests map cleanly to retargeted gun motions (`2e00`, `2e40`–`2e4f`).
- **Continuous Stance Hold:** The rifle remains drawn and active across burst delays, queued rounds, and ready windows while moving, allowing seamless firing while running or sprinting.

### 2. Strict On-Screen Camera View Targeting
- Enemies behind the camera or off-screen are strictly excluded.
- Targeting requires:
  - Positive camera dot product (`camDot > 0.5f`), confirming targets are strictly in front of the camera.
  - Camera view-cone alignment (`camDot / dist >= 0.45f`).
  - Screen projection bounds validation (`screen.w > 0.0f`, `screen.z >= 0.0f`, `0 <= x <= width`, `0 <= y <= height`).
- Raiden will never shoot behind the screen/camera unless locked on.

### 3. Multi-Enemy Spray Sweeping
- When multiple valid enemies are visible within the camera view, `groupSpray = true` activates.
- Targets are sorted horizontally by screen X coordinate. Target selection dynamically sweeps across visible enemies using `SmgPolicy::SprayIndex(fire.ordinal, visible.size())`.
- Spread offsets are distributed across the cluster during firing bursts, providing active suppressive fire across the enemy formation.

### 4. Active Lock-On Tracking (Codex Pattern)
- Integrates `GetLockOnTargetInfo(Pl0000* player)`, evaluating `g_GameCamera.m_LockonSystem` (`m_TargetEntity`, `m_pTarget`, `m_vecPosition`) and `player->field_1370.m_TargetHandle`.
- When Lock-on is held alongside C:
  - Firing locks onto and actively tracks the target (`aimPoint = lockInfo.position`).
  - Disables group spray for pinpoint accuracy.
  - Snappy aim tracking (`followStep = 0.40f` in `Prepare`, `0.35f` in `Aim`) snaps the gun to the target while turning Raiden to face the locked enemy when stationary.


Runtime 19: all targets, including native and player lock handles, must pass positive camera depth, viewport bounds, alive and range checks. Unlocked groups sweep left-to-right-and-back across on-screen enemies; Shift allows group spray even with lock-on. Native running legs remain active under the gun upper-body overlay. Shots use the current visible target rather than a lagging barrel heading. This is viewport targeting, not wall-occlusion detection.

Runtime 20: movement flick plus C replaces Shift spray. Keyboard W/A/S/D taps and left-stick flicks share the existing eight-tick directional buffer. Flick while C is held also enters spray; the mode stays active until C release. Continuous movement with an expired flick does not enable manual spray. Shift is not read by the gun spray controller. Gesture, gun, pose and archive tests pass.

Runtime21 movement/aim repair: native pelvis and legs remain authoritative at all speeds, including the idle-to-run transition. Gun input reservation no longer edits the persistent player input snapshot. Chest yaw is bounded to 35 degrees from native player heading, with arm aim adding at most 25 degrees. Camera-visible target acquisition and final spray trajectories are restricted to the reachable 60-degree forward arc; turn Raiden toward targets outside this arc. Shift remains Blade Mode; flick + C remains spray. Automated matrix and policy checks pass; live gameplay still needs verification.

Runtime22 supersedes the freeze repair: removed the animation-request hook that still substituted gun donor MOTs into the native locomotion graph whenever the gun was selected. Native movement MOT/sequence pairs now remain together; only the post-tick visual upper-body matrix overlay uses cyborg gun samples. A routing regression guard prevents reintroducing native gun motion replacement or lower-body overrides.

Runtime23: muzzle flash ESP binds directly to the gun entity root bone in default FF and authored slot 0. Its position is a local barrel offset (up .01, forward .39), with zero local rotation; native attachment follows the animated weapon instead of leaving a world-space flash behind. Live visual verification remains pending.

Runtime24: removed the invalid FF bone attachment (native FF forces entity-origin attachment). Flash now receives the complete rendered barrel matrix translated to the shared bullet muzzle point, preserving roll in slides. Gun body overlays run only during recognized grounded locomotion; slide/event, attack, and aerial animations retain their native bones while firing remains available. Visual gameplay verification is pending.

Runtime 25:
- Barrel Muzzle Flash: Flash ESP transform is anchored directly to the rendered barrel tip and aligned with the bullet trajectory forward vector, eliminating displacement and direction mismatch across movement modes.
- Ninja Run Upper-Body Overlay: Gun aiming and firing pose overlays the upper body (chest, shoulders, arms, hands) during Ninja Run (STA_NINJARUN and sprint input) while Raiden's legs continue high-speed ninja run locomotion.
- Ninja Run Slide Disablement: When performing a slide in Ninja Run (GAME_SLIDER_NINJARUN_MODE or slide animation), the gun aiming pose is cleanly disabled and the weapon is hidden, allowing native slide animations to play uninterrupted.
- Reactive Dynamic Crosshair: The reticle projects in real-time to where the gun converges using camera direction and player angle (worldToScreen). The crosshair dynamically blooms/expands with movement speed and recoil bloom from firing, tightening when stationary or locked on. When locked on, a targeted ring indicates the pinned enemy.
- Bullet Spread: Implemented dynamic bullet spread dispersion based on movement (stationary vs run vs ninja run) and firing cadence/recoil bloom, matching the crosshair's dynamic state.
- Launch Shot on Lock-On + Fire: Holding the Lock-On button while firing C releases high-impact launcher rounds (SmgPolicy::Launcher) that propel hit enemies into the air with vertical launch speed (0.48f) and ground-offset separation, enabling immediate air combos.

Runtime 26:
- No Auto Lock-On: Free-aim by default. Enemies on-screen are never automatically snapped to or locked on unless the lock-on button (pad LB / key 'E' / camera target) is actively held. Crosshair remains centered in camera view with dynamic spread bloom.
- Manual Lock-On Tracking: When lock-on input is held, the system acquires and locks onto the targeted enemy, drawing the targeted lock ring on the reticle, focusing bullet spread, and enabling launcher rounds with Lock-On + C.
- Airborne Upper-Body Shooting Pose: The upper-body rifle aiming and IK pose now applies during jumps and falling (`player->isInAir()`), while Raiden's legs and pelvis smoothly play native jump and air physics animations.
- Ninja Run Slide Suppressed Pose: Both `Tick` and `PostTick` completely disable the gun pose, reset IK, and suppress weapon display during Ninja Run slides (`GAME_SLIDER_NINJARUN_MODE`, `GAME_IGNORE_PARKOUR_SLIDING`, and slide animation codes), ensuring clean native slide slides and kicks.


## Current muzzle and sword attachment repair

Native selector 25 retains its wp0040/0 shot effect, core/26 visible bullets and
impact profile 1286. Only the custom rifle shot callback temporarily gives the
native muzzle emitter the gun handle, root bone 0 and local offset (0,.01,.39).
All modified callback fields and player ownership are restored before the pool
continues. The native projectile/trail callback is unchanged.

While aiming, the rendered gun root stays world-fixed across the native effect
update; only body bones are restored at the beginning of each game frame. The
original gun root is captured once and restored on aim cancellation. Restoring
it to the native lowered pose between frames allowed attached flashes to jump
to the legs, even when the gun itself looked correct after the visual aim pass.

The cyborg body overlay excludes weapon/sheath markers 0x700, 0x701 and 0x710.
Sword parking rigidly rebases all sword bone matrices to the sheath socket,
including cached/world-fixed children, then restores every original matrix and
flag before native update. Native sword constraints and draw state are untouched.
Shared player skeleton storage is excluded from sword placement.

Validation: release build (zero warnings/errors), native call/asset-table checks,
720 pose frames including the native-effect gap and cancellation, 200 sword
apply/restore frames and cached-child rebasing checks pass. This revision
supersedes prior repairs. Visual confirmation remains pending.

### Firing and traversal controls

Hold C/LB for focused gun aim; press or hold LMB/RT to fire. Aim engages on
press, and releasing a short untriggered aim tap retains the quick burst.
Grounded Ninja Run can fire with aim plus a live locked target. Losing that
target cancels gunfire. Slides, obstacle vaults, wall traversal and Ninja Run
jumps block firing and gun poses. Ninja Run jumps stay blocked until landing,
even if sprint is released in midair. A second check after the native update
cancels a queued shot if traversal started during that frame; release aim
before rearming after an interruption. Sprint and jump inputs remain native.

Control regression checks: `tools\test-smg-controls.cmd`.
