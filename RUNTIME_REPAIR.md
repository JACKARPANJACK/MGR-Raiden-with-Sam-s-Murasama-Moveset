# Runtime repair, October 1, 2026

## October 3: complete effect bank routes and Sam Zandatsu

Generated SamEffectBanks.h from the local EFF headers: all 337 playable Sam,
93 boss Sam and 286 core effect IDs are available to native helper routing.
The previous playable table covered 176 IDs; 161 were missing and could fall
back to Raiden's model bank. Sam helpers now prefer pl1400 on overlapping IDs.
Boss EffectTrack references still use their explicit em0020 bank. Activation
waits for both Sam banks to register, preventing attacks from starting before
boss effects finish streaming.

Native Zandatsu state 0x100059 (native predicate RVA 0x6C3910) now retains the
Sam graph through combat QTE just like human/mech executions. Its DLC sequence
chain remains selected after leaving Blade Mode. Some shared execution/Datsu
motions are intentionally absent from pl1400; those use the verified shared
MOT with Sam's authored sequence, as native DLC does.

Validation: 716 complete bank entries, 35 runtime sequence adapters, human/mech
execution and Zandatsu handoff guards, all 16 suites, and Win32 Release build.
Live effect appearance and paired animation completion remain unverified.

## October 3: native Sam execution QTE preservation

The blanket QTE safety handoff destroyed Sam's state graph during native human
and mech executions. EncounterPolicy now distinguishes SDK states 0x100066
and 0x100067 from campaign story QTEs. Those two combat QTEs keep Sam's
controller, animation map, effect routing and sheath management; event, codec
and soft-event flags still require a campaign handoff. No forced target
teleports, instant-kill substitutes or replacement enemy state graphs added.

Compared the local Sam-to-Raiden reference route at vtable slot 0x150
(campaign interaction dispatcher 0x7F5690), which the current per-player table
already inherits. Added checks for both execution states, retained story
handoffs, blocked addon throws, and all seven shared execution motion routes.
The existing fixture checks verify the shared Raiden MOT / Sam DLC sequence
pairs. Paired enemy positioning and prompt timing remain unverified in-game.

## October 3: charge and attack effect coverage

Imported charged, sonic, stone and Judgement Cut windups use Sam's native
charge start (117), mature glow (118) and release flash (119). The mature glow
starts once after 45 animation frames, or halfway through shorter windups.
Release fades the windup controller before emitting the flash; interruption
only fades, so damage/cutscene transitions do not emit a false release.
Sonic slash's 3010 windup was missing from the charge effect path.

Extended the effect audit to all rapid follow-up stages and Raiden specials
using their actual adapters. Checked 35 adapted sequences, 262 sequence effect
references and 484 native helper bank routes against local EFF headers. No
missing assets in executable mod sequences; 28 engine alias references remain
dependent on native alias resolution. All 16 suites and Win32 Release build
passed. In-game appearance remains unverified.

## October 3: stretched body and attack grip correction

The prior hand-grip binding reversed the entities in constraint 17, attaching
Raiden's body to the sheath. Rechecked the executable at RVA 0x478EA0-0x478EBE:
x86 pushes rotation 10, bone 0x701, sheath, player entity, and ID 17, in that
order. The SDK signature therefore requires the player as attachment parent
and the sheath as attached object. Corrected both the binding and the existing
constraint identity check; hand offsets remain native zero values.

Playable rapid/spinning slash sequence adaptation had also removed FlagsTrack,
including the authored grip windows. It now retains only flag 21 (hand grip)
and flag 53 (sword parking), with original timing. Other sequence flags remain
filtered, and boss/Raiden sequence adaptation keeps its previous behavior.
Management still runs through 1-GameFileFlagCheck.h.

Validation: native executable call order and SDK signature, all 16 regression
suites including all four rapid stages' flag masking/timing, and Win32 Release
build. Live gameplay and the screenshot pose remain unverified.

## Current update: actual sheath texture crash repair

The latest session logs repeat access violations in d3dx9_43.dll+0xDA849,
whose instruction probes the image buffer header. Installed RaidenRemaster
pl1404.wta advertised 5,776,286-byte and 5,587,078-byte textures, with the
second at offset 5,779,456. Its paired WTP is only 701,952 bytes and exactly
matches stock data107. This was an invalid mod DAT/DTT pairing, not evidence
that the existing 0x7F0 hip offset needed changing.

Backed up pl1404.dat and replaced only pl1404.wta with the original data107
table. Both entries now fit and have DDS headers; all other archive members
remain byte-identical. Installed pl1404.wmb also exactly matches stock Sam.
Verified the active pl1010 body's 0x7F0 rest transform and parent against Sam.
The runtime now uses the native Sam_Sheath entity name (RVA 0x46EC82 string),
logs successful attachment, and stops per-frame retries after a swap fault.

Repair command: python tools/repair_sheath_textures.py GAME_DIRECTORY --install.
The script requires byte-identical stock WTP before applying a stock table,
backs up before writing, and refuses installation while the game runs.

Validation: Release build, sheath/toggle/Round Trip suites, reproduction of
rejected original texture bounds, repaired DDS ranges, preserved model/archive
members, and installed costume bone check. No live gameplay verification yet.

## Current update: player-local temporary Sam sheath swap

Removed active-mode global costume-table writes. Activation swaps the live
player entity to pl1404, preserving the original costume sheath entity, model
ID and constraint 3 bone/rotation/offsets. Each active update checks the entity
and mount identity, retries unloaded/failed Sam creation, and hides the retained
original. The original stays at its native mount while Sam's resources load.
Repeated activations recapture the restored mount rather than a Sam fallback.
Deactivation, weapon changes and native story suspension restore the captured
Raiden sheath; scene reconstruction does not inherit a patched costume table.
Animation and grip flags remain managed through 1-GameFileFlagCheck.h.

Validation: Win32 Release build plus sheath, toggle and Round Trip regressions.
Live visual swapping/costume transitions remain unverified in-game.

## Current update: sprint priority, move configuration and Sai pull

Ninja Run input and native Sam dash/run attack/charge states suppress and clear
buffered directional add-ons. Walking retains all sixteen directional inputs.
Unsupported/air/Blade Mode contexts reset the gesture buffer. Corrected the
supplied INI to all nineteen catalog entries, sixteen unique mappings, and
playable rapid stages 9100/9101/9102/9108. The loader supports Playable/Followups
and rejects out-of-range directional and ultimate indices.

Added F/L3-alone Sai pull while Raiden's Sai is selected. The native action
hook receives a heavy trigger; after twelve ticks the nearest eligible cyborg
within fifteen units is pulled horizontally toward Raiden for at most thirty-six
ticks, stopping at 1.5 units. An actor-local speed unit is restored on completion,
weapon change, unsupported state, air/Blade Mode, scripted action, death or
shutdown. em0170 heavy hammer cyborgs, bosses, robots and body parts are excluded.
No extra damage is injected. Movement uses the engine character controller but
navigation/obstacle behavior needs gameplay verification.

SAM_MOVELIST.md documents native, sprint, directional, ultimate, Round Trip and
Sai inputs. Validation: Release Win32 build and sixteen suites, including actual
INI loading, all sixteen gesture paths, sprint policy and Sai exclusions/limits.
Live combat, controller timing and pull appearance remain unverified in-game.

## Current update: file flag sheath management

SheathController delegates flags 53 (blade parking), 21 (grip attachment),
QTE sheath map selection, and direct/polled sheath animations to BxmFlagHandle
in 1-GameFileFlagCheck.h. Its animation cache resets on player/sheath changes
and toggle-off. Removed unrelated native weapon/effect tick calls from these
helpers. Round Trip skips sheath management while the sword is in flight.
Corrected flag 21's identical branches and reversed grip attachment.

Validation: Release Win32 build and sheath/Round Trip regression suites.
Visual appearance and sequence-flag transitions require an in-game check.

## Current update: sheath grip and distinct directional attacks, October 3, 2026

Corrected constraint 17 to match native RVA 0x478EA0-0x478EBE: the
scabbard is the attachment parent and the player is the constrained object.
The scabbard grip marker supplies bone 0x701 and rotation bone 10, with zero
additional offset/rotation. Constraint 3 retains the hip mount at 0x7F0.
Grip constraints are checked for entity identity and repaired after native
rebinding. The retained original sheath is hidden each active update to
prevent native visibility changes from displaying two scabbards.

All sixteen direction/button/held-or-flick combinations now select distinct
moves and release clips. See DIRECTIONAL_MOVES.md for controls. Forward flick
plus Light restores the full playable Sam rapid/spinning sequence:
pl1400_9100 -> 9101 -> 9102 -> 9108. These stages explicitly use the playable
archive, preserve its native attack IDs/effects, and preflight all four pairs
before starting. Each stage advances at its MOT duration. X specials remain
separate from directional inputs.

Validation: Win32 Release build and fourteen regression suites, including
32 catalog stages, sixteen distinct input mappings, four rapid clips with
native hit numbers, and the shared grip binder tested with engine doubles.
Visual sheath alignment and attack appearance remain unverified in-game.

## Previous build: encounter and heatblade repair

Diagnosis from process 23312's October 1, 16:17 crash dump: native RVA 67C8A0
reads address 0x48 with ECX=0, called at 7BD989 during sword construction.
The engine log requests nonexistent pl0001/pl0004 after scene cleanup/startup.
Removed ChangeModelID's global costume-table and sheath-instruction writes.
Native costume construction now survives style changes without those mutations.
Sam's GitHub combat graph, charge, animation pairs and sequence logic are retained.

Verified the native DLC initializer also changes global sound, effect-attribute,
and effect-bullet selectors. Registration now restores all seven affected scene
selector/context words as well as DLC mode. Explicit native EFF/EFT registration
at A00C50 loads pl1400, em0020, em0220 (Bladewolf), and wp0372 banks. Successful
registrations acquire references; A00D60 balances them before archive references
are released at player shutdown. Animation-map references and readiness caches
are also balanced/reacquired across scenes.

Story events, QTEs and codec transitions temporarily use the native Raiden graph
and input. Sam selection remains requested; the restored Sam graph resumes once
native control returns to a supported action. Ordinary pause/hit-stop does not
tear down Sam. Boss parts/controllers keep native effects, collision and AI.

Added Bladewolf heatblades to the weapon cycle and secondary menu. It is a
projectile selection, never a melee entity attached to constraint 5. C or the
controller subweapon button throws wp0372 toward a nearby enemy with Wolf's
native 0.38/250 trajectory and BF4/BF8/BFC setup. Friendly player owner/handle and
collision filters are built from initialized descriptors; enemy flags are not
copied. Throws use a 24-frame cooldown, preserve the current Sam combat graph,
and do not consume grenade ammunition. Existing grenade-based kunai remain.
Projectile/controller shutdown is explicit and native controllers are unlinked
before storage is destroyed. Secondary requests now stream selected/pending
weapons instead of preloading every melee weapon; attachment caches are updated.

Validation: Win32 Release build, all 14 suites, native executable contract checks,
and effect coverage (449 sequences / 247 references). No missing references in
executable mod clips; stock Wolf em0220_0050 references absent EST 69, reported
separately and never executed on the player. Engine aliases and live encounters,
cutscenes, scene reloads, controller input and visible throws need a gameplay run.
Pre-change source snapshot: backups/encounter-heatblade-20261001-163126.

## Previous build: GitHub Sam restoration (3d65f1a)

Restored Sam's moveset from the fetched origin/main backup at commit
3d65f1a071bf905b6dc29541589c91ce64185131 ("commmit 2"). All 58 restored source,
UI, documentation and test files were written directly from Git blobs and
verified byte-for-byte. This restores Sam's native runtime/state graph, original
activation and G-toggle behavior, charge controller, animation/sequence routing,
electric combat, directional/ultimate stages, resource manager, sheath logic,
kunai-to-Sam bridge and supporting player/action dispatch. The recent automatic
resets, sheath-allocation activation gate, charge overrides and added combat
callbacks/effect calls are removed.

Retained only the isolated optional-weapon null guard and its x86 regression
test alongside the original eight slow-rate null guards. No Sam combat logic
is changed by these call-site guards. The two custom sheath test harness files
from the later rewrite were archived and removed from the active test tree.

The pre-restoration working copy, staged/unstaged binary patches and restoration
manifest are saved in backups/github-sam-restore-20261001-161113. The Git index
was preserved. Local body meshes and ModLoader assets were not rewritten.

Validation: Win32 Release build, the backup's fourteen-suite test runner and
native executable hook verification. A fresh gameplay run remains necessary to
verify the restored moveset with the user's installed asset/mod combination.

## Previous build: weapon-parent-repair-11

Investigated the latest local crash dump (process 6472, October 1 at 15:50).
It faults at native RVA 67C946 reading address 0x2C. The stack contains
68B120 -> 68C624 -> 7E91FD: secondary constraint construction passed a null
entity and tried to copy its handle. Weapon changes now remove constraint 5,
release the previous secondary entity before allocating its replacement, and
synchronize cached bone field_1420 with m_CustomWeaponBone. The exact native
7E91F8 attachment call additionally skips missing parent/child entities using
an x86 guard that preserves the original five-argument stack cleanup.

Sword constraint 4 belongs to the player or sheath behavior. Sheath swaps now
remove both previous owners before changing constraint 3. They do not remove
constraint 4 from the blade behavior. Tracking an unchanged native sheath no
longer invalidates the sword cache. The broken sword-placement approximation
has been replaced by the verified native Raiden/Sam placement routines, which
interpret the five animated 0x720 marker states and their different bones.
Native model creation again uses its original 0x710 ADD operand, preserving
the conditional 0x711 prologue attachment.

The game log also recorded repeated texture/entity allocation failures.
Secondary resources now stream only for the requested weapon; unused plugin
requests are released after replacement. Failed entity creation falls back to
Raiden sword. Failed Sam sheath creation retries at most three times, separated
by 120 ticks. A deferred Sam activation no longer repeats a destructive weapon
transaction each frame. The underlying live texture failure still needs a
new gameplay run to determine whether it persists.

Validation: actual x86 null-parent/null-child attachment guard over 10,000
iterations; actual sheath-controller parent cleanup, native-cache preservation
and bounded allocation-failure tests; native executable placement/call-site
verification; complete automated suite and Win32 Release build. Gameplay
appearance and a new weapon-change run remain unverified.

## Previous build: kunai-sheath-stability-10

Quick throws use C or the mapped subweapon button, only with Stun/Explosive/Heat
selected. Q remains weapon cycling. The raw controller Start bit is no longer
interpreted as LB. Projectile creation waits for streaming, enforces a 14-tick
cooldown and 32-shot limit, and reserves/restores native input around TickGame
so a press cannot also enter grenade aiming. Native inventory mode stays native.
Quick throws consume no inventory; aimed native releases retain consumption.

Removed the erroneous NOP patch at 7A44CA: this is Entity::getBehavior, not ammo
subtraction. Removed all kunai writes to player+0x1400, which is sword state.
Quick descriptors now use a real owner EntityHandle and native Raiden friendly
collision filters. Explosive payloads use native grenade attack/object metadata
and preloaded grenade resources. All variants age out, release their projectile
entities, and clear tracking/status effects on actor shutdown.

Sam mode creates a separate pl1404 sheath only after its resources and the body's
0x7F0 bone are ready. Native costume tables and original sheath archives remain
native. Constraint 3, both sheath references, and sword-parent cache change
together. Disabling Sam restores the original model, bone, rotation bone and
constraint transforms. Handle-based tracking prevents stale sheath pointers;
repeated toggles reuse one custom model, and player shutdown releases that model.
Native model creation continues to use Raiden's back bone 0x711. Activation waits
for a valid Sam attachment before installing the Sam combat controller.

Verified all 15 installed Raiden/title body meshes: 0x7F0 resolves through the
WMB4 three-level lookup table, has the donor parent ID, and preserves Sam's rest
transform. The merger now validates these conditions before preparing archives.
Existing costume sheath archives match their original backups. No new permanent
sheath aliases are installed.

Validation: Release Win32 build, sixteen automated suites including the actual
sheath controller compiled with engine doubles, native EXE hook/collision checks,
and installed body skeleton inspection. Appearance, projectile trajectories and
live combat/costume transitions still require in-game playtesting.

## Previous build: sam-body-merge-9

Used the installed ModLoader WMBEditors bone merger in interactive mode to add
Sam's missing native skeleton bones to all 14 installed selectable Raiden bodies
and the title body (15 meshes). Original bone records, geometry and unrelated
archive members were verified unchanged. The logical pl0010 archive has no body
mesh; pl1020 has no independent archive in this installation.

Sam's actual scabbard bone is 0x7F0, not 0x710. The sheath controller and native
constraint operand now use 0x7F0. Model selection changes only the sheath column
of the 14 native costume rows and restores their captured original IDs on exit;
costume hair, visor and face IDs are preserved.

Installed Sam's sheath geometry and matching texture banks under pl0103, pl0115,
pl1013, pl1083 and the CustomBodyParts pl10a3 alias. Original sheath animations
remain available. Added texture members and renamed aliases use rebuilt DAT CRC
lookup tables. Local copyrighted assets and backups remain excluded from Git.
Reapply with `python tools/merge_sam_bodies.py GAME_DIRECTORY --install` while the
game is closed; this requires the installed WMBEditors and local CPK index.

Validation: installed archive hashes, original-bone/geometry preservation, native
DAT CRC algorithm comparison, fifteen test suites and Release Win32 build.
Attachment appearance and costume transitions still require in-game validation.

## Previous build: smooth-switch-8

Added native DirectInput mouse-wheel cycling (up previous/down next), with partial
notch accumulation and latest-selection buffering. Q/E and D-pad Left/Right
remain available. Preloads and retains all four secondary/unarmed weapon archives
for the current player's lifetime; Sam DLC assets are also requested ahead of
selection. Duplicate selections no longer recreate equipment. Sam's ordinary
ground attacks allow swaps during their last six recovery frames; charge holds,
Blade Mode, Round Trip, ultimates, flight and scripted states remain guarded.

Controller B/Circle now queues the same lightning ultimate as keyboard X and is
reserved from native gameplay actions in eligible Sam combat states. Corrected
the earlier controller-toggle masks: the SDK's B is 0x20, while Select/Back is
0x200 and L3/R3 are 0x1000/0x8000. B no longer toggles Sam off. Menu, scripted and
QTE inputs retain their native behavior. Input state is restored after the native
tick; menu visibility uses an atomic flag across render and game threads.

Validation: Release Win32 build and fourteen suites, including wheel remainder,
wraparound, recovery guards and B press-edge/context tests. Native switch latency,
attack transitions and controller feel still require in-game testing.

## Previous build: encounter-unarmed-7

Three recent game dumps (28744, 1264 and 26300) show the same access violation:
native slow-motion propagation at RVA 7871BF reads EAX+4F0 with EAX=0 while the
optional custom-weapon handle is absent. Fixed all eight matching dereferences
in the immediate/deferred slow-rate routines at 787120/787260. Each exact-byte
checked guard returns a null destination for an absent Behavior, allowing the
existing native TEST/JZ to skip only that weapon. Native player/weapon slow-rate
timing and the rest of damage processing stay in the original routines.

Unarmed now selects native custom weapon ID 5 and streams/attaches wp2040, which
contains the 2cxx unarmed motions/sequences, effects and sounds. Sets native
unarmed sword state 1 (also used by the game's prologue setup), preserving sword
and sheath entities. Removed the setSwordLost(TRUE) path, which hid the sword
and set sword-lost state 2.

Validation: crash-dump exception/context analysis, executable-backed checks of
all eight native guard sites and unarmed ID/state, wp2040 asset fixtures, Release
Win32 build and fourteen automated suites, including actual x86 null-load/stack
regressions. The patched encounter and unarmed
transitions still require a new in-game run; this is not a playtest result.

## Previous build: kunai-subweapon-6

Added an independent Native/Stun/Explosive/Heat-blade subweapon selector on F7/F8
and the mod menu. Uses grenade inventory slot 1, native ammo consumption and
Raiden's aiming/throw animations. Returning to Native restores the earlier slot,
including RPG/Stinger. All projectile variants use wp0372 streamed through the
engine and the post-create setup verified in Bladewolf's sub_16E2F0. The function
itself is not called on Raiden because its actor fields are incompatible.

Only Raiden's grenade-release factory call at 7A4883 is patched, after verifying
its original target. Friendly owner/collision flags and aiming vectors are
preserved. Native knife impacts trigger actor-local EMP stun, an actual grenade
payload with reduced explosion power, or three bounded heat-damage pulses.
Explosive projectiles also release their payload on disappearance or a timed
fuse. Sam's native graph is suspended only from neutral grounded states during
kunai aiming; animation routing and add-on inputs are suspended with it. Sam
returns after native recovery. Existing electric stun timers keep advancing.

Validation: thirteen suites, a Release Win32 build, wp0372 model/effect/collision
fixtures and executable-backed factory/heat-blade contract checks. Runtime
collision dispatch, ammo, throw animations, EMP/heat trails, explosive blast
behavior and Sam controller restoration have not been playtested.

## Previous build: weapon-input-5

Q/E and D-pad Left/Right cycle all six weapons. Murasama is now a separate
selection from Raiden's sword: selecting it enables Sam, and switching away
disables Sam and cancels any pending activation. D-pad Up selects Murasama;
Down selects Raiden's sword. G/menu Sam toggles update the displayed selection.
Bracket keys remain available. Switching is queued during attacks/flight/Blade
Mode; QTE, story, paused and unfocused inputs do not queue accidental selections.
Normal-gameplay Q/E and D-pad input is reserved for the switcher during the native
tick, with shared input state restored afterwards. Input conflicts and native
equipment behavior still require in-game verification.

## Previous build: weapon-dlc-4

Integrated weapon selection on the game tick: Sword/Murasama, Pole-arm, Sai,
Pincer blades and Unarmed. Use the menu or bracket keys. Streaming completes
before equipping; switches wait through attacks, Blade Mode, flight and scripted
states. Native secondary weapons and unarmed punches exit Sam mode. G restores
the sword controller. Weapon creation/attachment follows the supplied native
switcher reference without its worker thread.

Sam animation routing now includes native DLC non-combat codes used for charge,
Iai, executions and Zandatsu instead of excluding everything outside 2xxx and
Blade Mode. Seven DLC execution sequences correctly accompany shared native
Raiden motions; other clips require matching DLC motion/sequence assets. Sam's
native state graph and callbacks own Blade Mode charge and Datsu. F uses Sam's
sweeping combat finisher rather than alternating with a lightning ultimate.

Release Win32 build and twelve suites pass, including 91 DLC pairs and seven
shared execution motions verified against actual installed assets. In-game
weapon behavior, paired enemy animations and visual alignment remain unverified.
The source tree is Git-ready with portable SDK/vendor paths, licenses, test-asset
preparation and installation tools. Local backup archives preserve removed
duplicate/reference projects and obsolete global-remapping code.

## Previous build: effects-repair-3

The effect callbacks now preserve the engine's distinct owner and controller
semantics, including actor, sword and sheath parents. Sam's effect bank and
descriptor metadata apply only during each synchronous native effect call;
Raiden's model index is restored immediately afterwards. Shared Raiden effects
retain their original bank unless they are one of the verified charge helpers.
Old mappings to nonexistent effects 539/540/541/550 and the speculative paired
slash-ID table have been replaced with scoped, asset-backed charge routing.

Boss leap effects now run on the player layer. The charged-combo recovery's
missing Sam effect 146 uses the installed charged-release flash 119. Charged
slash and lightning-storm windups have an explicit Sam charge effect, faded on
release, cancellation, G-off and player reset. Other native sequence effects
keep their original banks, timing, offsets, scale and control types.

Validation: Release Win32 build and eleven automated suites pass. The asset
audit reads 330 sequences / 174 effect references; actual C++ adapter output for
29 sequences has no missing Sam/boss/core EST references. Ten original 10010-bank
references remain under native engine alias resolution and are not certified by
the offline audit. `Release/effect-coverage.json` records the full coverage and
explicitly sets `gameplayVerified` to false. Effects have not been visually
playtested by Codex. Restart the game to load this build.

The following notes describe the underlying runtime-repair-2 changes retained
in the current build.

This build replaces the previous combat extension. Restart the game before testing;
an ASI already loaded by the game will keep executing its previous version.

The main corrections are:

- Sam now owns the active update loops, full combat callbacks, Blade Mode/Iai/Datsu
  state graph, animation routing, and native `pl1400_battleParameter.bin` table.
  Raiden's original table, context, callbacks, map and Ninja Run rate return on G-off.
- Blade Mode and Zandatsu requests include their non-2xxx Sam animation codes.
  Shared charge helpers route to actual Sam charge effects 117/118/119/121;
  particle banks are selected from installed asset IDs.
- The Zandatsu target field is no longer treated as an ordinary combat target.
  Grabs, finishers, electric strikes and Round Trip find nearby live enemies in
  front of Raiden, including the final entity in the native entity list.
- Direct boss stages use the actual MOT frame count instead of Raiden's cached
  animation-map end flag. Throw release, grab impact and electric strike each
  fire once at their stage's frame threshold.
- Flight and grip transforms are applied to the behavior/physics object after
  the native update, preventing weapon attachments/enemy movement from undoing them.

Grabs only hold ordinary humanoid cyborgs within 2.5 units. They acquire an
actor-local slow-rate unit, keep their original unit alive, pull the enemy to
Raiden's grip, then apply one bounded damage impact at frame 20 of the release.
The hold releases on interruption, death, QTE, Blade Mode or G-off. It can last
up to 180 active ticks to cover the windup, but ends at impact. The short physical
hold does not include a new paired cinematic victim animation.

X still cycles thunder slice, storm and electric draw. The release stage delivers
one ranged electric impact against a live enemy within 12 units. Core effect 316
is a real installed electric/plasma impact particle; native owner metadata is
supplied before spawning it. Particles start immediately and their controllers
survive stage completion. These reuse MGR effects rather than importing MGS4 assets.

Round Trip acquires a real enemy, detaches the visible sword after the native tick,
updates its behavior transform, and homes towards the enemy. It hits at most eight
times during a flight, with a 16-tick hit interval and a 2.5-unit contact range.
Each contact is capped at 2% of enemy max HP, so a complete flight is at most 16%.
It returns automatically after 130 ticks, with a 260-tick catch failsafe. Heavy,
X or T recalls after 25 ticks. The log records actual catch duration/hit count.

Attack playback and Ninja Run are 20% faster. Blade Mode charge timing is retained.
Sam's correct attack table replaces the oversized Raiden entries previously used
for Sam attack numbers. Ordinary combat collision power is reduced to 85% and
capped at 20 light / 35 heavy / 60 charged. Against a nearby ordinary cyborg,
per-hit caps are about 3.3% / 5% / 8.3% of max HP. Imported stages divide a 20%
HP allowance over their damaging attack boxes. Scripted executions and native
Blade Mode/Zandatsu remain governed by Sam's native logic.

Validation: Release Win32 build and ten automated suites, including real MOT
stage durations, native Sam table entries, damage budgets and immutable sequence
adaptation. Engine interactions and visual alignment have not been playtested by
Codex. The startup log identifies this version as `runtime-repair-2` and prints
Sam light/heavy/charged base powers. Stage logs print duration and hit-box count;
grab impact and Round Trip catch logs distinguish contact from a missed target.


## Native knife inventory and Raiden ultimate resources (2026-10-01)

The previous heatblade mode was a plugin selection with unlimited quickthrows.
The native DLC3 item list defines `DLC3_BladeKnife`, alias `154b4aab`, type 14,
ID 44, maximum 10. The existing base-game weapon menu already contains this
alias at menu entry 9 and maps it to equipped subweapon 10. The plugin now
provides the missing definition in supported Raiden scenes, creates the genuine
`cItemPossessionWeaponHeatKnife` through native factory `551C80`, and inserts it
through the native possession manager. Engine cleanup owns the allocation.
The definition uses the verified native 0x48-byte item schema, including the
engine-parsed `it0640` pickup object, ID 44, alias, capacity 10, GetPoint 100,
DLC3 display-name key and Param_1 = 1. Alias and ID lookup hooks support native
pickup/refill lookup without changing another item. Existing
DLC definitions win when available. The Raiden alias helper `77F840` now resolves
slot 10 to the knife. The D-pad reaches the original inventory menu.

Throwing requires an equipped, usable native knife item. A successful projectile
spawn calls the item's native `use()`; empty inventory and failed spawns consume
nothing. Selecting the heatblade shortcut no longer releases a melee entity.
Capacity is granted only when a new item is created, never every tick. Save-file
formats remain native; inventory reconstruction may grant the initial stock when
the base-game save does not contain a knife entry. No gameplay save test performed.

The three Raiden ultimates previously requested Sam boss `3016/3017`, `3500/3506`,
and `3010/3017`. They now use `pl0010` animation-map entries and complete native
motion/sequence pairs `2400`, `3501`, and `2420/2422`. `10010` archive requests and
pins follow scene lifetime. Adaptation retains Raiden attack numbers, banks,
layer flags and audio/visual timing while removing graph-owned flag tracks.
Collision lookup scopes Raiden's saved battle parameters and original vtable
attack function to each native call, restoring Sam parameters afterwards.
Sequence cache keys include archive identity. Boss add-ons keep the Sam adapter.

Validation: Win32 Release build; 14 regression suites; four native Raiden stages
checked against their animation map, attack IDs and immutable visual/audio tracks;
EXE contract verification for the knife class, ammo-use chain, factory, native
menu table and slot mapping. Gameplay, particle rendering and native inventory
visuals have not been verified.


## Precision aim, charged payloads and throws during combat (2026-10-01)

All kunai variants now use native knife subweapon 10. The previous grenade-state
bridge is removed: charging and spawning projectiles never replace Raiden/Sam's
melee state, animation map, battle table or weapon attachment. Native grenade
release remains untouched in behavior. A supported Raiden allocation is required;
Bladewolf/Sam campaign players retain their original controllers.

Native camera unprojection provides the raised precision ray. Holding C or the
mapped subweapon button for six ticks shows the reticle, charge bar, payload,
ammo and lock count; shorter taps use Raiden's forward target aim.
Precision acquisition scores angle first within a narrow cone; full-charge
volley acquisition widens the cone and chooses up to three distinct enemies.
This adapts campaign-style aiming without invoking Pl1500 methods on a Raiden
allocation (the native campaign knife routine accesses fields at 5718-5724).
No native camera state/factory is replaced.

Release-to-fire thresholds are 18/45 active ticks for stun/explosive. Reaching
90 active ticks automatically fires a ten-knife fan using the selected payload.
Full charge latches until C is released, so holding or releasing after that fan
cannot produce another shot. Every successful
spawn consumes one inventory item. Resource failure, empty inventory and shot
capacity cap cannot spend excess ammunition. Hit-stop freezes charging; scripted
states/death/focus loss cancel and suppress firing until the button is released.
Ground/air combat and Blade Mode permit independent knife throws; forced
execution graphs and story transitions retain native input. Six-tick shot spacing
allows ten knives in 54 active ticks, followed by a 120-tick recovery. Each volley
knife counts toward the burst limit; an empty partial inventory also triggers
recovery. Recovery resets the burst budget, without generating native ammo.
Up to ten releases are queued in order with their aim, payload and weak targets;
death, focus/story interruptions and inventory changes clear queued releases.
Charge/recovery timing uses gameplay ticks and freezes during hit-stop.

Held precision aim uses native pl0010_2561 (131 frames) at 2.5x speed, taking 53
active ticks with a five-frame entry blend. A successful throw replaces the aim
node directly with 2566 (86 frames) at 3x speed, taking 29 active ticks with a
three-frame transition blend. Pending releases retain the aim node, avoiding a
rest-frame gap before a cooldown-buffered throw. Held input cannot replace an
unfinished throw and rapid shots cannot restart it. Completion requests rest
map 4 only if the node is still ours and neutral; attacks and native movement
retain their interruption ownership. Settled aim nodes are thawed before the
throw transition. Tests verify both actual asset headers, duration conversion
and the aim/throw transition gates.
Rapid throws do not restart an active pose. Direct playback uses no sequence:
the original sequence's grenade-controller flags and any native ammo path are
excluded. Throws during air movement, combos, ultimates or Blade Mode preserve
the active motion. Neutral completion requests idle only while the recorded
Direct node remains current, using the previous locomotion map's native blend;
replacement attack/movement nodes retain control. Attack/jump/Blade Mode inputs
cancel pending pose recovery so it cannot overwrite their animation.
Fixture tests verify the native motion header and burst spacing, ten-shot budget,
partial inventory exhaustion, volley limits and paused recovery.

Precision aim activates only after C is held for six active ticks. Short taps
use Raiden's facing direction and forward target acquisition. Precision aim
shares normalized HUD coordinates (0.5, 0.44) with native camera unprojection
at 99FAB0, using the engine viewport width/height getters B98A90/B98AA0.
The last held ray, depth and weak targets are captured for release; later camera
movement never changes the flight. Native trajectory launch aims from Raiden's
muzzle toward the ray at target depth, with no chest snapping. Full charge uses
ten symmetric yaw offsets spanning -45 to +45 degrees, centered on that ray and
limited by native ammo, burst budget and projectile capacity. Fan projectiles
retain their initial spread for 18 ticks before enemy homing begins, so steering
does not immediately collapse the fan. Homing remains enemy-directed after launch.
Tests cover full-charge automatic firing and one-fan-per-hold suppression,
symmetric fan angles, ammo limits, and multiple convergence
distances, hold/tap boundaries, pose completion, no windup restart, paused pose
timing, interruption and replacement-node ownership. Native contracts verify
the actual viewport getters and unprojection ABI; visual alignment still requires
gameplay verification.

Homing keeps weak entity handles, reacquires a valid forward target after target
loss, caps turning at 0.16 radians per active tick, preserves native speed and
expires after 120 projectile ticks. It steers the BulletBase vector at 920 before
native physics, without teleporting projectiles or applying synthetic hits.
EXE checks verify that vector's launch, movement and rigid-body transfer sites.
Native collision still decides wall/enemy hits. wp1011 resources/effects are now
requested, pinned, registered and released alongside the scene; grenade payload
metadata uses verified native object 31011, behavior 26, type 11 and attack 57.

Verification includes Release build, 14 regression suites, real charge boundary/
cancellation and ammo limits, aerial/combo eligibility, angular acquisition,
constant-speed bounded homing (including a target passing behind), native EXE
contracts and installed effect assets. No gameplay/visual stability claim is made.
