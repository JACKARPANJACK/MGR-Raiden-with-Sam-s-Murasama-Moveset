"""Extract required test fixtures from the user's local game installation."""
import argparse
from pathlib import Path
from assets import cpk_index, decompress, dat_entries, write_local

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_data',type=Path)
p.add_argument('--output',type=Path,default=Path('local_assets'))
a=p.parse_args()
required={'pl/pl1400.dat','pl/pl1404.dat','em/em0020.dat','core/coreeff.dat','pl/pl0010.dat','wp/wp0372.dat'}
shared={'8550','8552','8553','8585','8960','8970','8980'}
found=set()
for archive in sorted(a.game_data.glob('*.cpk')):
    rows=cpk_index(archive)
    with archive.open('rb') as stream:
        for row in rows:
            if row['path'].lower() not in required:
                continue
            stream.seek(row['offset'])
            blob=decompress(stream.read(row['size']))
            base=archive.stem+'/'+row['path']+'.unpacked/'
            for name,data in dat_entries(blob):
                if row['path'].lower()=='pl/pl0010.dat' and name.lower() not in {'pl0010_'+code+'.mot' for code in shared}:
                    continue
                write_local(a.output,base+name,data)
            found.add(row['path'].lower())
            print('Extracted fixtures:',row['path'])
if required-found:
    raise SystemExit('Required archives absent: '+', '.join(sorted(required-found)))
