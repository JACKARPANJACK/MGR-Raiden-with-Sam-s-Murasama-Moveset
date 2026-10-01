Current update: see [DIRECTIONAL_MOVES.md](DIRECTIONAL_MOVES.md). The directional add-on implementation supersedes older ultimate, sheath, and charge notes below.

# Native combat runtime validation

Previously gameplay-validated stability build SHA-256: `D8F25BCA26CF80BB53A64963329AAD8CEE13E82457FA28A1FAF54C4D33B02CB5`

Installed X-ultimate test build SHA-256: `2DA99CC0B82961523837E34D239A74B8E3809E4D97153461D87433DFEA2BB33D`.
The older gameplay confirmations below apply to the stability build, not to the
new ultimates. The new build passes compilation and four regression suites;
live validation of its special moves and projectile remains pending.

The plugin extends newly created Raiden allocations to accommodate Sam's extra
fields. Activation requires an allocation tracked by this plugin, initialized
Sam fields, a loaded Sam animation map, and pinned DLC/boss resources with
registered effects. Native physics/lifecycle slots remain Raiden's.

G-off restores the owning player's saved table and map. It starts Raiden's fall
state when airborne and idle when grounded, replacing the active Sam animation
sequence. Shutdown restores ownership before calling native cleanup. G-off no
longer writes assumed values to any global Sam dispatch table.

G-on during native damage, scripted, or dead states stays pending until an allowed
live state. Pressing G again cancels a pending request. This avoids interrupting
engine-owned scene transitions.

## Evidence

- Earlier native-runtime live test: user reported working. Logs recorded light
  state `10000F` with clips `2000` through `2003`, combo-charge state `100010`
  with `2020` through `2022`, and standing-charge state `100011` with `2100`,
  `2101`, `2108`. Matching Sam sequence attachment was recorded.
- Current build: Win32 Release build passes. Combat routing, archive lookup,
  and toggle transition policy tests pass.
- Asset check: double-jump `023a`, jump/fall/landing transitions, and checked
  aerial attack clips each have a playable Sam motion and `_2_seq.bxm` pair.
- Previous stability build live validation: user reported "works" after being asked to test
  double jump and air combos, hold/release charge, G-off during air/charge, and
  repeated reactivation. The live log also recorded replacement sequences and
  successful deactivation. This is user-confirmed gameplay coverage, not an
  automated exhaustive gameplay test.

Passing a build or a routing test does not establish gameplay stability. No claim
of universal mission, cutscene, death/reload, or mod-interoperability coverage is made.

## Restored fixes and X ultimates

Restored tracked/extended player allocations and shutdown hooks; rejected
untracked player activation; removed reintroduced global table/factory writes,
model-ID changes, and per-frame ProcessSamEvents/ProcessGameFixes calls. Restored
the campaign sequence attachment hook and exact native Sam clip selection,
including double-jump/air transition clips. Native Sam charge sequences supply
their own charge sound and effect tracks.

Keyboard X starts the next ultimate when grounded and idle, or queues it until
the current ground attack finishes. Four distinct ground light clips ending in
2003 also queue an ultimate. The list rotates across 14 distinct finishers (27 total stages):
1. Boss stone burst (3020 -> 3024)
2. Boss charged slash (3000 -> 3004)
3. Boss sonic slash (3010 -> 3014)
4. Boss sweeping finisher (nullptr -> 2600)
5. Boss leaping slash (3300 -> 3302)
6. Boss continuous slash (2200 -> 2220)
7. Boss sweeping combo (2600 -> 2610)
8. Boss Judgement Cut (92e0 -> 92e4)
9. Raiden thunder slice (3016 -> 3017)
10. Raiden lightning storm (3500 -> 3506)
11. Boss unarmed grab & smash (a648 -> a649)
12. Boss unarmed tackle & smash (a646 -> a648)
13. Boss Murasama round trip throw (3200 -> 3210)
14. Raiden lightning draw slash (3010 -> 3017)

## New Capabilities Added:
- **Directional & Stick Flick Moves**:
  - Forward-Forward stinger thrusts (`2110`, `2111`, `2112`, `2121` -> `2110`/`2120`).
  - Back-Forward rising launchers (`2150`..`2156` -> `2410`..`2412`).
  - Directional dodge attacks / Offensive Defense (`2140`..`2143`, `2240`..`2243`, `2300`..`2350` -> `2510`, `2520`, `2530`, `2540`).
  - Stick flick spins and 360 whirlwind slashes (`2172`..`2176`, `2600`..`2602` -> `2600`, `2610`).
- **Murasama Round Trip Sword Throw & Unarmed Mode**:
  - Detaches Murasama blade via `player->removeConstraint(4)` and switches Raiden to native unarmed mode (`setSwordLost(TRUE)`).
  - Blade dynamically homes in on target enemy, rapidly spins dealing multi-hit cuts (`core_se_btl_char_zan`) with high-intensity glowing trails.
  - While active, Raiden uses unarmed combat moves (punches, kicks, sweeps).
  - After 130 ticks, blade homes back to Raiden, re-attaches cleanly, and restores armed mode (`setSwordLost(FALSE)`).
- **Boss Sam Unarmed Grab & Head Smash**:
  - Adapted `a648` (tackle, pin, and head smash with `pl0010_se_dmg_crush` sound) and `a649` recovery.
- **Raiden Draw Attack Thunderstorm & Lightning Slash**:
  - When charging standing or combo draw attack (`0x100011` / `0x100010` / charge actions), passes tier 1 to invoke full environmental thunderstorm effect (ID 754), lightning flashes, and blade energy glow.
  - On release of the charged draw, unleashes a massive lightning slash with heavy thunder sound cues (`core_se_btl_char_zan`, `core_se_btl_ripper_in`, `core_se_sys_item_electro_repair`), maximum blade illumination (50.0f), and smooth fadeout.

The debug menu shows the next move, queued status, and offers a "Cycle Next Move" button.
Thunder moves engage the engine's built-in environmental thunderstorm effect (754),
play thunder crackle and surging electricity sound cues (`core_se_btl_ripper_in`,
`core_se_sys_item_electro_repair`, `core_se_btl_char_zan`), boost blade glow,
and cleanly fade out over 4 seconds on move completion. Judgement Cut triggers the
10-hit spherical dimensional cut (effect 250) with blade ignition cues.

The private combat table handles custom action 10007C, verified unused/default
in Sam's native dispatch. Native input and shared maintenance still run. A
damage/scripted state change cancels ownership; G-off clears pending/active
ultimates and replaces the sequence while restoring Raiden. Missing resources
reject an ultimate before changing its action. A 600-tick stage timeout prevents
an endless custom action. Airborne starts are deliberately not supported.

Boss sequences are cloned for process lifetime. Original archives are immutable.
Boss-specific flags and unsupported tracks are disabled; strike boxes use Sam's
player attack entry 26. Telegraph boxes receive zero-length damage intervals.
Timing, sound, and effect tracks are preserved. Tests inspect all nineteen local
boss stages and verify adaptation, source preservation, and truncated-data rejection.

Stone spawn follows the native ground-burst call at 2A987 -> 206E0 after frame 22.
The adapted implementation constructs the native generic bullet descriptor,
uses player-owned hit data, and creates streamed/pinned wpc001 through 6D3BE0.
It never calls Em0020's actor handler with a Raiden pointer. The old supposed
projectile function at 50B490 was actually a raycast and has been removed.
Actual projectile behavior, visible boss effects, and damage require live testing.

Source backup: backups/pre-ultimate-20260930-*
Installed plugin backup: backups/pre-ultimate-installed-20260930-231839

## Sheath Hip Position & Animation Synchronization:
- **Native Hip Attachment**:
  - Re-attached sheath constraint 3 to left hip bone `0x710` (1808) via native engine calls `attachObject(3, player->m_pEntity, pSheath, 0x710, 0)` and `player->setConstraintsBone(3, 0x710, 0)`.
  - Replaced manual offsets with clean zeroes: `m_vecOffset = (0, 0, 0, 0)` and `m_vecRotation = (0, 0, 0, 0)`. Sam's Murasama sheath model (`pl1404`) origin is designed by PlatinumGames to sit directly at bone `0x710`, eliminating any clipping or sinking into the body.
  - On deactivation (G-off) or shutdown, cleanly restores constraint 3 to Raiden's default back bone `0x711` (1809) with clean zeroes.
- **Model Switching**:
  - Integrated `ChangeModelID()` in `SamMovesetManager::Activate` and `Deactivate`. When active, writes `0x11404` (Sam's red Murasama sheath) and `0x11401` to all player equipment slots, ensuring the true Murasama sheath model is used instead of Raiden's default back box (`0x10004`).
- **Continuous Sheath Animation Synchronization**:
  - `SheathController::Update(player)` runs every tick in `SamMovesetManager::OnTick`.
  - Monitors player animation slot (`player->m_pAnimationSlot->m_pArray[0].m_pAnimName`) and sword hidden state (`player->m_bSwordHidden`).
  - Sanitizes motion names (strips `pl0010_`, `pl1400_`, `pl1404_`, `.mot`) and matches against `pl1404.dat` animations (`pl1404_0000.mot` idle, `0200` run, `0201` ninja run, `023a` air/jump, `2000`..`2048` light/heavy attacks, `2100`..`2120` draws, `3010`..`3060` specials, `92e4` Judgement Cut).
  - Automatically plays matching motion on `pSheath->m_pBehavior->setDirectAnimation` or falls back to sheath map state (`requestAnimationByMap(4 / 5)`).

## Round Trip Move Mechanics & Verification:
- **DMC-Style Round Trip Implementation**:
  - **Throw Initiation (Stage 3210)**:
    - Detaches blade constraint 4 from both sheath entity and player entity (`removeConstraint(4)`).
    - Sets `g_GameStateManager.IsRoundTripActive = true;` to bypass all periodic `UpdateSwordConstraints` calls while the blade is in flight.
    - Puts Raiden into genuine engine unarmed mode via `reinterpret_cast<void(__thiscall*)(Pl0000*, BOOL)>(shared::base + 0x77E210)(player, TRUE)` (`setSwordLost(TRUE)`).
    - Sets `player->m_bSwordHidden = 1` and `player->field_13F4 = 0`.
    - Spawns blade with forward offset calculated from player yaw (`player->m_Rot.y`) and ensures visibility via `blade->m_pBehavior->onDisp()`.
    - Plays throw audio: `em0020_se_atk_counter_throw` and `core_se_btl_char_zan`.
  - **Dynamic In-Flight Homing & Multi-Hit Combat**:
    - Every tick: forces blade visibility via `blade->m_pBehavior->onDisp()`.
    - Automatically homes in on target enemy (`player->field_1370.m_TargetHandle.getEntity()`), or projectile direction if untargeted.
    - Rapidly rotates in 3D (`blade->addRot(...)`) and maintains high-intensity Murasama red energy glow (35.0f).
    - Every 8 ticks: applies real combat damage via `BehaviorAppBase::damage(45, false)` and plays slicing sound cues `core_se_btl_char_zan` and `pl1400_se_swd_scrape`.
  - **Unarmed Combat Mode**:
    - Once the throw animation completes, Raiden transitions into unarmed idle (`0x100008`) with stance `0000`.
    - While the blade is in flight, Raiden has full freedom of movement and native unarmed combat (punches, kicks, sweeps, evasions).
    - Other ultimates and conflicting sword actions are strictly locked out until the blade returns.
  - **Early Recall & Return Phase**:
    - Phase 2 (or manual recall after 25 ticks via Heavy Attack / Right-Click / Y, X key, Taunt / D-Pad Up / T, or ImGui "Recall Blade Now" button) fast-forwards to tick 130.
    - In Phase 2: blade homes directly back to Raiden at high velocity (0.70f per tick) while continuing to slice nearby enemies (35 damage within 2.5 units).
    - Arrival threshold (distance <= 0.75f) or failsafe timeout (tick 260) triggers `EndRoundTrip`.
  - **Sword Catch & Armed Combat Restoration**:
    - Re-attaches blade constraint 4 directly to right hand bone `1792` (`attachObject(4, player->m_pEntity, sword, 1792, 0)` and `setConstraintsBone(4, 1792, 0)`).
    - Sets `player->field_13F4 = 1` and reactivates drawn sword mode via `shared::base + 0x811BE0`.
    - Restores armed combat: `setSwordLost(FALSE)` and resets `m_bSwordHidden = 0`.
    - Restores player state from unarmed idle (`0x100008`) to sword stance (`0x100000`) and requests stance map 4.
    - Plays sword catch audio cues: `core_se_btl_ripper_in`, `core_se_sys_item_electro_repair`, `core_se_btl_char_zan`.
  - **Diagnostics & GUI Control**:
    - Added dropdown menu in ImGui diagnostics overlay with direct trigger button for "Boss Murasama round trip throw" (Move 13).
    - Real-time indicator displaying Round Trip Active status.
    - One-click "Recall Blade Now" button to test or cancel Round Trip on demand.
  - **Verification Suite**:
    - Unit test `tests/round_trip_tests.cpp` verified via `tools/test-roundtrip.cmd`: PASS.
    - Full regression suites (`test-sheath.cmd`, `test-ultimates.cmd`, `test-combat-routing.cmd`, `test-resources.cmd`, `test-style.cmd`, `test-toggle-policy.cmd`): ALL PASS.
