"""Add Sam's missing bones to installed Raiden body DATs, preserving skin data."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
from assets import dat_entries
from merge_sam_bodies import bones, bone_index, pack_members, validate_sheath_bone

ROOT = Path(__file__).resolve().parents[1]
BODY_MARKERS = {0, 1, 2, 0x100, 0x200, 0xf, 0x13, 0x700}

def ids(records):
    return [struct.unpack_from('<h', b)[0] for b in records]

def merge_model(before, donor):
    old = bones(before)
    source = bones(donor)
    old_ids, donor_ids = ids(old), ids(source)
    assert len(set(old_ids)) == len(old_ids)
    missing = [i for i in donor_ids if i not in old_ids]
    if not missing:
        validate_sheath_bone(before, donor)
        for i in donor_ids:
            bone_index(before, i)
        return before, []
    records = list(old)
    indices = {i: n for n, i in enumerate(old_ids + missing)}
    for bone_id in missing:
        donor_index = donor_ids.index(bone_id)
        record = bytearray(source[donor_index])
        parent = struct.unpack_from('<h', record, 4)[0]
        parent = -1 if parent < 0 else indices[donor_ids[parent]]
        struct.pack_into('<hh', record, 2, len(records), parent)
        records.append(bytes(record))
    # Three-level 12-bit native bone-ID lookup, with contiguous level arrays.
    all_ids = ids(records)
    assert all(0 <= i <= 0xfff for i in all_ids)
    high = sorted({i >> 8 for i in all_ids})
    middle = sorted({i >> 4 for i in all_ids})
    table = [-1] * (16 + len(high)*16 + len(middle)*16)
    for n, group in enumerate(high):
        table[group] = 16+n*16
    for n, group in enumerate(middle):
        table[table[group >> 4] + (group & 15)] = 16+len(high)*16+n*16
    for index, bone_id in enumerate(all_ids):
        table[table[table[bone_id >> 8]+((bone_id >> 4)&15)] + (bone_id&15)] = index
    result = bytearray(before)
    result += b'\0' * (-len(result) % 16)
    bone_offset = len(result)
    result += b''.join(records)
    result += b'\0' * (-len(result) % 16)
    lookup_offset = len(result)
    result += struct.pack('<' + str(len(table)) + 'h', *table)
    struct.pack_into('<4I', result, 0x3c, bone_offset, len(records), lookup_offset, len(table)*2)
    result = bytes(result)
    # Every original byte outside the four skeleton header fields is untouched.
    assert result[:0x3c] == before[:0x3c] and result[0x4c:len(before)] == before[0x4c:]
    assert bones(result)[:len(old)] == old
    for bone_id in all_ids:
        assert bone_index(result, bone_id) == indices[bone_id]
    for record in records:
        parent = struct.unpack_from('<h', record, 4)[0]
        assert -1 <= parent < len(records)
    validate_sheath_bone(result, donor)
    return result, missing

def audit(game, install=False):
    mods = game/'mods'
    donor = dict(dat_entries((mods/'RaidenMoveset/pl/pl1400.dat').read_bytes()))['pl1400.wmb']
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    backup = ROOT/'backups'/('skin-sam-bones-'+stamp)
    reports, changes, accessory_profiles = [], {}, set()
    for archive in sorted(mods.rglob('pl*.dat')):
        if archive.parent.name.lower() != 'pl':
            continue
        original = archive.read_bytes()
        members = dict(dat_entries(original))
        replacements = {}
        for name, model in members.items():
            if not name.endswith('.wmb') or model[:4] != b'WMB4':
                continue
            if not BODY_MARKERS.issubset(ids(bones(model))):
                accessory_profiles.add(archive.relative_to(mods).parts[0])
                continue
            merged, added = merge_model(model, donor)
            if added:
                replacements[name] = merged
            reports.append(dict(archive=archive.relative_to(game).as_posix(), model=name,
                beforeBones=len(bones(model)), afterBones=len(bones(merged)),
                addedBoneIds=[hex(i) for i in added], samSheathBone='0x7f0',
                nativeBoneLookupVerified=True, originalBonesAndGeometryPreserved=True))
        if replacements:
            packed = pack_members({n: replacements.get(n, b) for n,b in members.items()})
            after = dict(dat_entries(packed))
            assert after.keys() == members.keys()
            assert all(after[n] == replacements.get(n,b) for n,b in members.items())
            changes[archive] = (original, packed)
    if install:
        running = subprocess.run(['tasklist','/FI','IMAGENAME eq METAL GEAR RISING REVENGEANCE.exe','/FO','CSV','/NH'], capture_output=True,text=True)
        assert 'METAL GEAR RISING REVENGEANCE.exe' not in running.stdout, 'Close the game before patching skins'
        for archive,(original,packed) in changes.items():
            assert archive.resolve().is_relative_to(mods.resolve())
            saved = backup/archive.relative_to(game)
            saved.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(archive,saved)
            assert saved.read_bytes() == original
            archive.write_bytes(packed)
            assert archive.read_bytes() == packed
    profiles = sorted({r['archive'].split('/')[1] for r in reports})
    result = dict(bodyModels=reports, bodyProfileCount=len(profiles), bodyProfiles=profiles,
                  inheritedBodyProfiles=sorted(accessory_profiles-set(profiles)),
                  patchedArchiveCount=len(changes), installed=install,
                  backup=str(backup) if install and changes else None, gameplayVerified=False)
    result['archiveHashes'] = {r['archive']: hashlib.sha256((game/r['archive']).read_bytes()).hexdigest() for r in reports}
    out = ROOT/'Release'/('installed-skin-bones.json' if install else 'installed-skin-bones-audit.json')
    out.write_text(json.dumps(result,indent=2)+'\n')
    print(f'PASS: {len(reports)} body models across {len(profiles)} profiles; {len(changes)} archives '+('patched' if install else 'need patching'))
    print('Shared-body profiles:', ', '.join(result['inheritedBodyProfiles']) or '(none)')
    print('Report:',out)
    return result

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path)
    parser.add_argument('--install',action='store_true')
    args=parser.parse_args()
    audit(args.game.resolve(),args.install)
