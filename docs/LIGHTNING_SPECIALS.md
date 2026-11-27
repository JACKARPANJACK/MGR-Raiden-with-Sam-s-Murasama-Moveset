# Lightning specials

Equip the Murasama / Iaido katana, open the menu with **0**, choose a move
under **Select Move**, and press **Trigger Now**. In combat, **X / controller B**
queues the selected special using the existing ground attack recovery rules.
**Cycle Next Move** advances the selection. The eight additions occupy reserved
move indices 20–27 and are appended to the selector even with an older INI.
Existing directional mappings and the original three specials remain available.
The original three lightning entries now migrate from their old aerial/kick
placeholders to grounded Sam animations. Explicitly different custom mappings
are preserved.

For a visual check, press **0** and use **Lightning visual** near the top of the
menu, then **Preview lightning (no damage)** or **Preview hand charge**. These
buttons close the menu and play the effect without enemies or combat damage.

| Move | Behavior |
| --- | --- |
| Chidori | One close frontal electric thrust, 4m reach |
| Kirin | Windup, then a 4m burst around a forward enemy within 18m |
| Chidori Nagashi | Three electric pulses around Raiden, 5m radius |
| Chidori Senbon | Electric needle-style fan hitting up to six enemies within 16m |
| Double Chidori | Two close frontal electric impacts, 4m reach |
| Chidori Sharp Spear | Narrow piercing line, 14m reach |
| Chidori sword wave | Windup followed by a widening electric wedge, 14m reach |
| MGS4 lightning discharge | Four surrounding pulses, 8m radius |

Visuals use a dedicated native particle bank, **pl1c30.eff / pl1c30.eft**,
inside `pl1c30.dat` / `pl1c30.dtt`. The bank contains 18 EST definitions:
eight hand charges, eight releases, an impact and a Kirin miss. Four custom
DXT5 lightning sprites provide jagged cores, branches and alpha glow. Native
EST emission delays, angles, positions and lifetimes animate them. There is
no lightning overlay or custom Direct3D renderer.

The loader registers the bank on demand with the game's native effect
manager. It does not register the custom bank at player initialization. Atlas
900 is reserved for the custom sprites; the builder rejects a retail atlas ID
such as 000. Replacing atlas 000 interfered with existing particles in testing. Controllers are created and released on the game thread. Charge
particles use the native matrix override for wrist 13; Double Chidori also
binds wrist 9. The parent descriptor keeps its bone field at -1: the native
spawn function rejects other values. Kirin binds
to its target or uses a separate effect eight metres ahead for a harmless
miss. Native controller cleanup precedes scene bank release.

`tools/build_lightning_effects.py` authors the resources from a complete native
blue billboard record (core 118/0), retaining its renderer fields. The older
316/3 donor had extra render selectors absent from this bank. Particle
lifetimes now exceed the native fade-in/fade-out interval. It writes only the custom bank;
it does not replace the retail core effect bank. Installation retires previous
core overrides only when their hashes match this mod's generated versions,
and backs them up first. Restart the game after installation.

Pulse timing follows the first damaging AttackTrack event, excluding long
animation recovery tails. Fan/line/wave damage resolves in the move's defined
geometry at release; particles do not add projectile collision or wall
raycasts. No imported Naruto/MGS4 animation or dragon model is included.

Damage is capped per enemy across each move's scheduled pulses (up to 60 HP or
roughly 12.5% of maximum HP, subject to native power and integer rounding).
Only ordinary cyborgs receive bounded electric stun. Bosses keep their native
controllers. Private animation sequence copies disable native sword collision
damage for these moves so the scripted pulses are the sole damage source.
Death, air transitions, damage states, Blade Mode, pause, focus loss and scripted
events stop future pulses. Pulses require live targets for damage; Kirin still
shows its cast on a miss. `scripts/LightningRuntime.log` records the native
bank, EST ID, parent bone and whether the engine accepted each spawn. The
menu displays accepted/rejected spawn counts. Acceptance confirms engine
creation, not final visibility or placement.

Run `tools/test-lightning.cmd` for native EST/texture structure, all effect
IDs, timed emissions, texture variants, deterministic packing, particle-only
routing, target shapes, authored hit timing, damage and INI migration. Final
visibility and bone placement require an in-game check; automated format
checks do not establish that particles rendered correctly.

EST field-layout reference: [F-SERVO's effect reader](https://github.com/ArthurHeitmann/F-SERVO/tree/master/lib/fileTypeUtils/effects).
