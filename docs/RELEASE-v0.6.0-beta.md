# Raiden Moveset v0.6.0-beta — Sam-compatible suits

Complete installer with the current Sam/Murasama moveset, guns, pistol combos,
CQC/stealth, Sai extensions and lightning specials.

Includes `Scripts/CustomSuitSlot.asi` and its default configuration, alongside
MGS2 Raiden, Raiden Coat, Maiden Raiden, Sequel Raiden, Regular Fatigues,
Raiden Sunglasses, MGS4 Raiden Alt and Raiden Remaster.
`Scripts/MGR-Unlimiter.asi` is included because Raiden Remaster declares it as a dependency.

All 23 body models passed Sam bone-ID lookup and sheath-bone validation.
Sunglasses inherits its body. Existing geometry and skin bones were preserved
during adaptation. Raiden Remaster supplies shared base-body replacements.

Extract the **Complete.zip** into a separate folder and run `Install.cmd` while
the game is closed. An existing working MGR ModLoader installation is required.
The installer verifies file hashes, backs up replaced files, registers the
profiles and preserves existing settings and profile order. CustomSuitSlot uses
the suit registrations included with the costume profiles.

Use the accompanying `.sha256` file to verify the ZIP. No unrelated ASI plugins,
ModLoader binaries, debug symbols or runtime logs are included.

This is a beta: structural and automated checks do not establish in-game
appearance, switching or combat correctness. Those gameplay checks remain pending.
