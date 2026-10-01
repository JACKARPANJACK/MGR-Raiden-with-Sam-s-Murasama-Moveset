# Runtime repair, October 1, 2026

## Current build: kunai-subweapon-6

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
