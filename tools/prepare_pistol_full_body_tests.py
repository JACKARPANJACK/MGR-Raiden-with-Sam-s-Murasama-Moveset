from pathlib import Path
import struct
from assets import dat_entries
from pistol_attack_catalog import PLAYER_CLIPS
from merge_sam_bodies import bones

root=Path(__file__).resolve().parents[1]
out=root/'Release/PistolBodyFixtures';out.mkdir(parents=True,exist_ok=True)
members=dict(dat_entries((root/'Release/GameData/wp/wp2062.dat').read_bytes()))
(out/'clips.txt').write_text(''.join(c['code']+'\n' for c in PLAYER_CLIPS))
for code in (c['code'] for c in PLAYER_CLIPS):(out/(code+'.fbw')).write_bytes(members['wp2062_'+code+'.gwm'])
records=bones((root/'local_assets/pistol-unity/installed-player.wmb').read_bytes())
with (out/'bones.txt').open('w') as f:
    f.write(str(len(records))+'\n')
    for r in records:
        i=struct.unpack_from('<h',r)[0];parent=struct.unpack_from('<h',r,4)[0]
        f.write(' '.join(map(str,(i,parent,*struct.unpack_from('<3f',r,8))))+'\n')
