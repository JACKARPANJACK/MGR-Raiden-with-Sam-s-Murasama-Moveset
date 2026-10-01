"""Use the installed WMBEditors bone merger on local ModLoader body archives.

Preserves existing model geometry and all unrelated DAT members. Game assets
remain local. Originals and an installation manifest are saved before writes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import datetime
import zlib
from assets import dat_entries, decompress

BODIES=('pl0010','pl0100','pl0110','pl1010','pl1020','pl1030','pl1040','pl1050',
        'pl1060','pl1070','pl1080','pl1090','pl10a0','pl1200','pl1210','pl1220','pl1300')
SHEATHS=()

def pack_members(entries):
    """Build DAT metadata including filename CRC lookup buckets.

    Format reference: Jacky720/MGR2Blender2MGR DAT exporter.
    https://github.com/Jacky720/MGR2Blender2MGR/tree/master/dat_dtt/exporter
    """
    names=list(entries);count=len(names)
    assert 0<count<32768
    width=max(len(n.encode('ascii'))+1 for n in names)
    offsets=32;extensions=offsets+count*4;name_table=extensions+count*4
    sizes=name_table+4+count*width;hash_map=(sizes+count*4+3)&~3
    shift=31-(count-1).bit_length();buckets=1<<(31-shift)
    hashes=[zlib.crc32(n.lower().encode('ascii'))&0x7fffffff for n in names]
    order=sorted(range(count),key=lambda i:hashes[i]>>shift)
    first=[-1]*buckets
    for position,i in enumerate(order):
        bucket=hashes[i]>>shift
        if first[bucket]<0:first[bucket]=position
    hash_values=16+2*buckets;indices=hash_values+4*count
    result=bytearray(hash_map+indices+2*count)
    struct.pack_into('<4s7I',result,0,b'DAT\0',count,offsets,extensions,name_table,sizes,hash_map,0)
    struct.pack_into('<I',result,name_table,width)
    struct.pack_into('<4I',result,hash_map,shift,16,hash_values,indices)
    struct.pack_into('<'+str(buckets)+'h',result,hash_map+16,*first)
    struct.pack_into('<'+str(count)+'I',result,hash_map+hash_values,*(hashes[i] for i in order))
    struct.pack_into('<'+str(count)+'h',result,hash_map+indices,*order)
    for i,name in enumerate(names):
        encoded=name.encode('ascii');start=name_table+4+i*width
        result[start:start+len(encoded)]=encoded
        ext=name.rsplit('.',1)[-1].encode('ascii');assert len(ext)<=3
        result[extensions+i*4:extensions+i*4+len(ext)]=ext
        result+=b'\0'*((-len(result))%16)
        struct.pack_into('<I',result,offsets+i*4,len(result))
        struct.pack_into('<I',result,sizes+i*4,len(entries[name]))
        result+=entries[name]
    assert dict(dat_entries(result))==entries
    return bytes(result)

def bones(blob):
    assert blob[:4]==b'WMB4'
    offset,count=struct.unpack_from('<II',blob,0x3c)
    assert count<4096 and offset+count*32<=len(blob)
    return [blob[offset+i*32:offset+(i+1)*32] for i in range(count)]

def bone_index(blob, bone_id):
    """Resolve the three-level WMB4 bone map used by native getPartsPtr."""
    assert 0 <= bone_id <= 0xfff
    table=struct.unpack_from('<I',blob,0x44)[0]
    assert 128 <= table < len(blob)
    def entry(index):
        assert index>=0 and table+index*2+2<=len(blob), 'Invalid bone lookup table'
        value=struct.unpack_from('<h',blob,table+index*2)[0]
        assert value>=0, f'Bone {bone_id:#x} is not mapped'
        return value
    mid=entry(bone_id>>8)
    leaf=entry(mid+((bone_id>>4)&15))
    index=entry(leaf+(bone_id&15))
    records=bones(blob)
    assert index<len(records) and struct.unpack_from('<h',records[index])[0]==bone_id, 'Bone lookup resolves the wrong record'
    return index

def validate_sheath_bone(blob, donor):
    records=bones(blob);source=bones(donor)
    index=bone_index(blob,0x7f0);donor_index=bone_index(donor,0x7f0)
    parent=struct.unpack_from('<h',records[index],4)[0]
    donor_parent=struct.unpack_from('<h',source[donor_index],4)[0]
    assert 0<=parent<len(records) and 0<=donor_parent<len(source), 'Invalid sheath parent'
    assert records[parent][:2]==source[donor_parent][:2], 'Sheath parent ID changed'
    # Translation/rest pose stays Sam's; parent indices may change on Raiden.
    assert records[index][8:]==source[donor_index][8:], 'Sheath rest transform changed'
    return index

def replace_members(blob,replacements):
    """Append replacements; preserve original metadata, hash table and members."""
    result=bytearray(blob)
    count,offsets,_,names,sizes,_=struct.unpack_from('<6I',result,4)
    width=struct.unpack_from('<I',result,names)[0]
    before=dict(dat_entries(blob))
    assert not set(replacements)-set(before)
    for i in range(count):
        name=blob[names+4+i*width:names+4+(i+1)*width].split(b'\0')[0].decode('ascii')
        if name not in replacements: continue
        result+=b'\0'*((-len(result))%16)
        start=len(result);data=replacements[name];result+=data
        struct.pack_into('<I',result,offsets+i*4,start)
        struct.pack_into('<I',result,sizes+i*4,len(data))
    after=dict(dat_entries(result))
    assert before.keys()==after.keys()
    assert all(after[n]==replacements.get(n,b) for n,b in before.items())
    return bytes(result)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path)
    parser.add_argument('--install',action='store_true')
    args=parser.parse_args();game=args.game.resolve()
    repo=Path(__file__).resolve().parent.parent
    profile=game/'mods/RaidenRemaster/pl'
    merger=profile/'WMBEditors/bin/bonemerge.exe'
    assert merger.is_file(), 'Installed WMBEditors bone merger missing'
    index=json.loads((repo/'local_assets/cpk-index.json').read_text())
    def source(name):
        local=profile/name
        if local.is_file(): return local.read_bytes()
        row=next((r for r in index if r['path'].lower()=='pl/'+name.lower()),None)
        if not row: return None
        with (game/'GameData'/row['archive']).open('rb') as stream:
            stream.seek(row['offset']);return decompress(stream.read(row['size']))
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    work=repo/'Release'/('sam-body-merge-'+stamp);work.mkdir(parents=True)
    backup=repo/'backups'/('sam-body-merge-'+stamp)
    donor=dict(dat_entries(source('pl1400.dat')))['pl1400.wmb']
    donor_path=work/'pl1400.wmb';donor_path.write_bytes(donor)
    assert any(struct.unpack_from('<h',b)[0]==0x7f0 for b in bones(donor))
    prepared={};reports=[];missing=[]
    for body in BODIES:
        original=source(body+'.dat')
        if original is None:missing.append(body);continue
        entries=dict(dat_entries(original));model=body+'.wmb'
        if model not in entries: missing.append(body);continue
        before=entries[model];old_bones=bones(before)
        target=work/(body+'.wmb');target.write_bytes(before)
        # The bundled executable's positional parser has a bug; its documented
        # interactive mode correctly merges and accepts piped local filenames.
        result=subprocess.run([str(merger),'-i'],cwd=work,input=target.name+'\n'+donor_path.name+'\n\n',
                              text=True,capture_output=True,timeout=60)
        assert 'Done!' in result.stdout and 'Traceback' not in result.stderr, result.stdout+result.stderr
        merged=target.read_bytes();new_bones=bones(merged)
        assert new_bones[:len(old_bones)]==old_bones, body+' original bones changed'
        assert any(struct.unpack_from('<h',b)[0]==0x7f0 for b in new_bones)
        sheath_index=validate_sheath_bone(merged,donor)
        # Vertex data and draw metadata before the skeleton tables remain exact.
        old_offset=struct.unpack_from('<I',before,0x3c)[0]
        translate=struct.unpack_from('<I',before,0x44)[0]
        geometry_end=min(x for x in (old_offset,translate) if x>128)
        assert before[128:geometry_end]==merged[128:geometry_end], body+' geometry changed'
        prepared[body+'.dat']=replace_members(original,{model:merged})
        reports.append({'body':body,'beforeBones':len(old_bones),'afterBones':len(new_bones),'samSwordBone':'0x7F0','sheathBoneIndex':sheath_index,'boneLookupAndParentVerified':True,'geometryPreserved':True})
        print(body,len(old_bones),'->',len(new_bones))
    for name,blob in prepared.items():(work/name).write_bytes(blob)
    manifest={'bodyReports':reports,'missingArchives':missing,'sheathAliases':[],
              'merger':str(merger),'visualsPlaytested':False,'files':[]}
    if args.install:
        # Never update a profile while the game's loader can be repacking it.
        running=subprocess.run(['tasklist','/FI','IMAGENAME eq METAL GEAR RISING REVENGEANCE.exe','/FO','CSV','/NH'],capture_output=True,text=True)
        assert 'METAL GEAR RISING REVENGEANCE.exe' not in running.stdout,'Close the game before installing body archives'
        backup.mkdir(parents=True)
        for name,blob in prepared.items():
            destination=profile/name
            assert destination.resolve().is_relative_to(profile.resolve())
            if destination.exists():shutil.copy2(destination,backup/name)
            destination.write_bytes(blob)
            digest=hashlib.sha256(blob).hexdigest()
            assert hashlib.sha256(destination.read_bytes()).hexdigest()==digest
            manifest['files'].append({'file':str(destination),'sha256':digest,'hadOriginal':(backup/name).exists()})
        manifest['backup']=str(backup)
    (work/'manifest.json').write_text(json.dumps(manifest,indent=2))
    print('Prepared',len(reports),'bodies with Sam sheath bone 0x7F0.',work)
    if args.install:print('Installed and verified. Backup:',backup)

if __name__=='__main__':main()
