# Raiden pistols

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
and `pistol_anims.dat`. Both DAT and DTT bytes are retained for the process
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

