# Pistol FBX coverage

All 150 package FBXs were inventoried. Retarget sources are exclusively from `Animation/Humanoid`.
The runtime bank contains 16 player attacks: eight attack families, each with its in-place and root-motion source variant.
Root travel remains actor-owned; authored body turns and vertical movement are retained.

| Humanoid family | In-place code | Root-motion source code | Destination |
| --- | --- | --- | --- |
| pistol_attack01 | 2f50 | 2f58 | wp2062.dat: directional combos |
| pistol_attack01_down | 2f51 | 2f59 | wp2062.dat: directional combos |
| down_shot | 2f52 | 2f5a | wp2062.dat: directional combos |
| melee_combat_01_1p | 2f53 | 2f5b | wp2062.dat: directional combos |
| melee_combat_02_1p | 2f54 | 2f5c | wp2062.dat: directional combos |
| melee_combat_03_1p | 2f55 | 2f5d | wp2062.dat: directional combos |
| melee_combat_04_1p | 2f56 | 2f5e | wp2062.dat: directional combos |
| throw_pistol | 2f57 | 2f5f | wp2062.dat: directional combos |
| contact | 2f60 | 2f65 | Local auxiliary review bank; excluded from player combos |
| melee_combat_01_2p | 2f61 | 2f66 | Local auxiliary review bank; excluded from player combos |
| melee_combat_02_2p | 2f62 | 2f67 | Local auxiliary review bank; excluded from player combos |
| melee_combat_03_2p | 2f63 | 2f68 | Local auxiliary review bank; excluded from player combos |
| melee_combat_04_2p | 2f64 | 2f69 | Local auxiliary review bank; excluded from player combos |

The eight paired `2p` motions animate the opponent. Both `contact` variants are retained as auxiliary gestures.
They are retargeted into `local_assets/pistol-unity/paired-and-contact-review.dat`, separately from the installed attack bank.

The 48 other Humanoid FBXs are non-attack idle, ready, equip, reload, pickup and locomotion clips; the Humanoid T-pose supplies the bind-calibration reference.
The 74 Generic FBXs and the mesh FBX are excluded. Per-file hashes and dispositions are in `local_assets/pistol-unity/fbx-coverage.json`.

Direction + C/LB starts one attack. A single buffered follow-up can change direction; repeated taps refresh it. Route variation advances across combos so all 16 attacks can be reached.
C+LMB / LB+RT retains regular shooting priority. Shot cues follow each clip and use the final animated muzzle direction.

Paired victim reactions and prop-throw gameplay are not implemented by retargeting the player body motion.
The pistol-throw body animation is used with the held pistol and the existing projectile volley system.

Validation: complete native MOT and full-body matrix agreement for every exported frame, continuous rotations, rigid skin twists,
Humanoid T-pose bind checks, no added source-rotation jumps across 3,458 frames, valid BXM attack/recovery spans, original pose/grip/attack byte preservation, route coverage, and 4,872 full-body playback frames at three costume scales, including entry and interstage blends.
The full-body source poses were observed in game in the previous build. The responsive timing profile still requires an in-game feel check.
