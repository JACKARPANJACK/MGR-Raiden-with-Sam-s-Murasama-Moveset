"""DAT additions and renamed texture banks must remain discoverable by the loader."""
import struct
import sys
import zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from merge_sam_bodies import pack_members, replace_members, bone_index, validate_sheath_bone
from assets import dat_entries

def verify(entries):
    blob=pack_members(entries)
    assert dict(dat_entries(blob))==entries
    offset=struct.unpack_from('<I',blob,24)[0]
    shift,buckets,hashes,indices=struct.unpack_from('<4I',blob,offset)
    names=list(entries)
    for name in names:
        crc=zlib.crc32(name.lower().encode())&0x7fffffff
        position=struct.unpack_from('<h',blob,offset+buckets+2*(crc>>shift))[0]
        assert position>=0
        found=False
        while position<len(names):
            h=struct.unpack_from('<I',blob,offset+hashes+position*4)[0]
            if h>>shift!=crc>>shift:break
            index=struct.unpack_from('<h',blob,offset+indices+position*2)[0]
            if h==crc and names[index]==name:found=True;break
            position+=1
        assert found,name
    return blob

verify({'pl0103.wmb':b'geometry','pl0103_2370.mot':b'native motion','pl0103.wta':b'new texture table'})
verify({'pl10a3.wtp':b'Sam texture payload'})
entries={f'member_{i:04}.mot':bytes([i%256])*i for i in range(257)}
blob=verify(entries)
patched=replace_members(blob,{'member_0010.mot':b'new larger skeleton payload'})
expected=dict(entries);expected['member_0010.mot']=b'new larger skeleton payload'
assert dict(dat_entries(patched))==expected
def skeleton(parent=0, mapped=1):
    blob=bytearray(128+64+96*2)
    blob[:4]=b'WMB4'
    struct.pack_into('<II',blob,0x3c,128,2)
    struct.pack_into('<I',blob,0x44,192)
    struct.pack_into('<hhh',blob,128,1,0,-1)
    struct.pack_into('<hhh',blob,160,0x7f0,0,parent)
    struct.pack_into('<h',blob,192+7*2,16)
    struct.pack_into('<h',blob,192+(16+15)*2,32)
    struct.pack_into('<h',blob,192+32*2,mapped)
    return bytes(blob)

assert bone_index(skeleton(),0x7f0)==1
assert validate_sheath_bone(skeleton(),skeleton())==1
for broken in (skeleton(mapped=-1),skeleton(mapped=0),skeleton(mapped=10),skeleton(parent=9)):
    try:
        validate_sheath_bone(broken,skeleton())
    except AssertionError:
        pass
    else:
        raise AssertionError('Invalid sheath skeleton accepted')
print('PASS: DAT CRC lookup, preserved archive members, sheath bone lookup and parent validation')
