"""Extract real native pairs for the controller integration regression."""
from pathlib import Path
from assets import dat_entries
from pistol_attack_catalog import PLAYER_CLIPS

root=Path(__file__).resolve().parents[1]
out=root/'Release/PistolControllerFixtures';out.mkdir(parents=True,exist_ok=True)
members=dict(dat_entries((root/'Release/GameData/wp/wp2062.dat').read_bytes()))
for clip in PLAYER_CLIPS:
    for suffix in ('.mot','.gwm','_0_seq.bxm','_timing.bin'):
        name='wp2062_'+clip['code']+suffix
        (out/name).write_bytes(members[name])
