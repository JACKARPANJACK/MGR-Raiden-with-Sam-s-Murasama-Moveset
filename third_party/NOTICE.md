This project includes modified Sam-in-Raiden campaign code by Frouk3 under the
MIT license, retained in `licenses/Sam-Port-MIT.txt`.

The weapon equip/attachment implementation is adapted from the locally supplied
MGRWeaponSwitcher project, under Apache License 2.0. Its original license is
retained in `licenses/WeaponSwitcher-Apache-2.0.txt`. The integration replaces its
worker-thread switching with a queue serviced on the game tick, adds unarmed
selection, and guards Blade Mode, QTEs, flight and active attacks.

Dear ImGui 1.91.8 is vendored in `imgui`, under its MIT license. The game, plugin
SDK, extracted game assets, reverse-engineering dumps and compiled plugins are
not included in the Git source tree.
