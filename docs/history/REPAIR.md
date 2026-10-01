Current runtime-only work is documented in RUNTIME_STATUS.md. The historical
deployment instructions below require external assets and do not satisfy the
runtime-only request. Do not deploy this experiment using those instructions.

The previously repaired build uses Raiden's native controller with Sam animation replacements
and Sam's actual pl1403 sword / pl1404 sheath. It enables automatically in the
main campaign. G requests a toggle, applied on the next idle/walk state. A full
game restart is required after replacing the plugin.

The former hybrid dispatch sent Raiden into unhandled Sam action states, wrote
past the 0x5400-byte Raiden allocation, and ran some actions twice. That path is
no longer compiled. Sam's native DLC controller and unrelated campaign hooks are
not applied to Raiden. Native combat rules remain in use; this is not a complete
port of Sam's DLC charge-state mechanics or the old experimental custom QTEs.

Animation maps are copied per player using their actual array size. Native
action IDs and cancellation/root-motion metadata remain intact. Only names with
available motion and required sequence files are replaced. The 48 current pairs
pass the installed-asset check. Idle 0200 stays native because 9200 has no matching
sequence. Active animation slot pointers are restored before the copy is freed.

Weapon replacement waits for both resources and preserves the original entities
and constraints for toggling back. Sheath animation starts on clip transitions,
with no forced 3x playback or per-frame visibility/offset reset. The repaired
pl1404 DAT retains its additional animations and uses the original texture table
paired with its original DTT. Original game CPK archives are not modified.

Build and install from this workspace (the older E:\Raiden Moveset checkout is
not synchronized):

1. Build Release|Win32 with Visual Studio 2022 / the configured MGR_PLUGIN_SDK.
2. Run tools\test-style.cmd from the workspace root.
3. Run python tools\prepare_repair.py.
4. With the game closed, run tools\deploy-repair.ps1. It backs up replaced files
   and verifies installed hashes. Build alone no longer overwrites the plugin.

Verification performed: native Win32 release build; production StyleSwitch tests
with SDK stubs for bounds, fallback, unchanged native metadata, shared resource
isolation, toggle, active slot lifetime and respawn; 48 motion/sequence pairs;
sword and sheath DDS texture offsets; executable hook-site inspection.

In-game behavior has not been verified. The remaining playtest is: light/heavy
chains, dodge/parry, jump/air attacks, movement, blade mode, damage interruption,
sheath draw/recovery, G toggling, and checkpoint/costume reload. These checks need
to confirm hit timing, rig alignment and compatibility with the other installed
plugins. Compilation and archive checks do not prove those visual behaviors.
