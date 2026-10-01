Current implementation: see [RUNTIME_REPAIR.md](RUNTIME_REPAIR.md). The runtime
repair supersedes the damage, grab, particle, charge-context and timing details
below; the directional control table remains valid.

# Directional boss add-ons â€” October 1, 2026

Enable Sam combat with G. Attack buttons use the game's configured Light/Heavy bindings.
Directions use the left stick; WASD also selects a direction on keyboard.
A flick is movement from the dead zone into a direction followed by an attack within
8 active ticks (about 133 ms at 60 Hz), including after releasing the stick.
Hold a direction beyond that window to use the held-direction move.

| Direction | Flick + Light | Flick + Heavy | Held + Light | Held + Heavy |
|---|---|---|---|---|
| Forward | Rapid slashes | JCE / Judgement Cut | Rapid slashes | Charged slash |
| Back | Grab and smash | Round Trip | Sweeping finisher | Stone burst |
| Left | Sweeping combo | Sonic slash | Sweeping combo | Sonic slash |
| Right | Tackle and smash | Leaping slash | Leaping slash | Round Trip |

All eleven boss moves are excluded from the X ultimate selector. X cycles only
Raiden thunder slice, lightning storm, and lightning draw slash. Completing four
light attacks no longer adds an ultimate automatically. Neutral attacks, air
combat, and simultaneous Light + Heavy retain native Sam handling.

Add-on requests expire after 18 active ticks. They start at idle or a finished
attack, or in a native ground attack cancel window. Damage, aerial transitions,
and scripted actions cancel queued requests. Both motion/sequence stages are
checked before taking ownership of the action. Missing stages keep native combat.
Boss flag tracks stay disabled; visual, sound, speed, and vibration tracks are
retained. Boss attack boxes use Sam's player damage table, not boss dispatch.
Grab/tackle animations use these adapted attack boxes; they do not run boss QTEs.

Sheath handling now maps Sam charge clips to the matching scabbard clips, parses
prefixed motion and sequence names, avoids duplicate starts from the animation
hook, and only resets constraint transforms when rebinding. Original costume
attachment transforms are restored on G-off. Hip placement remains bone 0x710;
visual alignment still needs an in-game check with the installed costume/assets.

Effect registration must succeed before activation. Shared charge and attack
helpers translate the reference Sam effect IDs only for the active player and
known caller ranges. Native sequence effect IDs are retained. The Raiden
lightning overlay was removed from normal Sam charging; lightning remains on
the Raiden ultimate moves.

Round Trip waits until frame 12 of its release animation to launch (a timing
setting that needs visual validation). It retains blade visibility, removes hand
and sheath constraints during flight, and damages a live target only within 2.5
units of the blade. Return movement is capped to avoid overshoot. Heavy, X, or
taunt recalls after 25 ticks; automatic return starts at 130 ticks with a 260-tick
catch failsafe. GUI recall catches immediately. Catch restores the armed blade;
G-off, death, and lost blade references also clean up flight state.

Validation: Release Win32 compilation and eight local regression suites pass.
All 27 catalog stages have local motion/sequence assets and pass immutable boss
sequence adaptation checks. Policy tests cover flick consumption/expiry, held
inputs, ultimate exclusion, proximity damage, no-overshoot return, and effect
actor/caller isolation. These are automated code/asset checks, not gameplay
verification. Check hit timing, charge effects, sheath alignment, Round Trip
launch/recall, G toggling, and checkpoint reload in the game after a full restart.

## Combat extension

F alternates a sweeping finisher and an electric draw finisher when the selected
ordinary cyborg is alive, within 3 units, and at or below 25% HP. These are
combat animations with native attack boxes, not cinematic executions or guaranteed
instant kills. Native Zandatsu remains available through Blade Mode.

While G is active, the native Sam initializer (RVA 0x493B60) supplies his factory,
context, Blade Mode charge/Iai nodes and Datsu Jump/Short nodes. His runtime type
descriptor is installed for native Sam node casts. Toggle-off exits/destroys that
state graph and restores the saved Raiden graph/context/factory; each activation
constructs a fresh Sam context. Extended allocation remains required.

Regular attacks have a 12% electric stun chance; heavy moves have 25%. A proc
requires a confirmed HP decrease and a native hit owned by Raiden, within 5 units
of the selected target. It lasts 45 active ticks with a 180-tick cooldown. This
is an EMP-style mod stun using electric rain particles and a temporary actor rate;
it does not invoke the grenade's native EMP status. Bosses, detached parts,
shared slow-rate units, scripted slows and QTEs are excluded. Only the selected
target is monitored. Misses and damage from other actors cannot roll a proc.

Back/right Flick + Light grabs now hold a nearby ordinary cyborg for up to 30
ticks and bring it to the grip position. Holds release on interruption, Blade
Mode, QTE, death and toggle-off; the imported sequence supplies smash damage.
These are short physical holds rather than boss-specific paired QTE animations.

Lightning ultimates summon the game's native Raiden electric-rain particles on
the selected target as well as the player. These are MGS4-inspired effects built
from MGR's assets, not imported MGS4 effects. No extra damage is added for particles.

Imported rapid slashes use Sam light attack No.4, sweeps No.10, ordinary heavy/
grabs No.12, charged strikes No.26. JCE repeated hits use No.4 and its boss No.14
finisher uses No.26. No.0 telegraphs remain disabled. Round Trip now uses native
player attack-power calculation (RVA 0x77ED30) instead of fixed 45/35 damage.

Validation: Release Win32 build and nine regression suites. Gameplay checks are
still required for native Blade Mode charge/Zandatsu, rapid damage balance,
selected-target hit attribution, grip alignment, effect appearance and cleanup
across toggle/checkpoint transitions. No game process was running during installation.
