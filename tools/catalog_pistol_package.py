"""Audit every FBX in the supplied package; treat paths/content as asset data."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--package',type=Path,required=True);args=p.parse_args()
    out=ROOT/'local_assets/pistol-unity/package-fbx';out.mkdir(parents=True,exist_ok=True)
    rows=[]
    with tarfile.open(args.package) as archive:
        paths={m.name.split('/')[0]:archive.extractfile(m).read().decode().splitlines()[0] for m in archive if m.name.endswith('/pathname')}
        for guid,name in paths.items():
            if not name.lower().endswith('.fbx'):continue
            path=PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts:raise ValueError('Unsafe asset path')
            data=archive.extractfile(guid+'/asset').read();target=out.joinpath(*path.parts)
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
            family=path.stem.removesuffix('_inplace')
            candidate=family.startswith(('pistol_attack','melee_combat')) or family in ('down_shot','throw_pistol','contact')
            group='generic' if '/Generic/' in name else 'humanoid' if '/Humanoid/' in name else 'mesh'
            rows.append(dict(path=name,local=str(target),sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),family=family,rigType=group,inplace='/inplace/' in name,attackCandidate=candidate))
    families=sorted({r['family'] for r in rows if r['attackCandidate']})
    report=dict(packagePath=str(args.package.resolve()),packageSha256=hashlib.file_digest(args.package.open('rb'),'sha256').hexdigest(),totalFbx=len(rows),attackCandidateFamilies=families,files=rows)
    (ROOT/'local_assets/pistol-unity/package-inventory.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'{len(rows)} FBXs audited and extracted; {len(families)} distinct attack/contact candidate families')
    for name in families:
        variants=[r for r in rows if r['family']==name]
        print(name,len(variants),'variants',len({r['sha256'] for r in variants}),'distinct binary payloads')


if __name__=='__main__':main()
