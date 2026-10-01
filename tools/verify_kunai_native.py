"""Check the kunai integration's native contracts against a local game EXE."""
import argparse
from pathlib import Path
import struct

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable',type=Path)
args=parser.parse_args()
data=args.executable.read_bytes()
pe=struct.unpack_from('<I',data,0x3c)[0]
assert data[pe:pe+4]==b'PE\0\0'
machine,count=struct.unpack_from('<HH',data,pe+4)
assert machine==0x14c, 'Expected the Win32 game executable'
opt_size=struct.unpack_from('<H',data,pe+20)[0]
base=struct.unpack_from('<I',data,pe+24+28)[0]
sections=[]
for index in range(count):
    offset=pe+24+opt_size+index*40
    size,rva,raw_size,raw=struct.unpack_from('<IIII',data,offset+8)
    sections.append((rva,raw_size,raw))
def read(rva,size):
    for start,length,offset in sections:
        if start<=rva and rva+size<=start+length:
            return data[offset+rva-start:offset+rva-start+size]
    raise AssertionError(f'RVA {rva:x} not mapped')
def call(rva,target):
    encoded=read(rva,5)
    assert encoded[0]==0xe8 and rva+5+struct.unpack_from('<i',encoded,1)[0]==target
call(0x7a4883,0x6d3be0) # Raiden grenade release, after native ammo logic
call(0x16e5e7,0x6d3be0) # Bladewolf heat-blade factory
assert struct.pack('<I',0x30372) in read(0x16e2f0,0x330)
assert struct.pack('<I',0x31011) in read(0x7a4410,0x4b0)
assert read(0x16e5f9,10)==bytes.fromhex('c780f40b000000000000')
for address,expected in ((0x1641b88,250.0),(0x1641bac,0.38),(0x163b5e4,0.5),(0x163baf8,0.3)):
    assert abs(struct.unpack('<f',read(address-base,4))[0]-expected)<1e-5
assert read(0x7a48b4,3)==bytes.fromhex('c20800') # __thiscall throw takes two args
print('Verified native Raiden grenade release, generic factory, wp0372 heat-blade setup and movement constants.')
for site in (0x7871bf,0x787208,0x78731d,0x78736e,0x787491,0x7874e2,0x7875ff,0x787650):
    esi=site==0x787208
    expected=bytes((0x8b,0xb0 if esi else 0xb8,0xf0,4,0,0,0x85,0xf6 if esi else 0xff,0x74))
    assert read(site,9)==expected, f'Unexpected native weapon guard site {site:x}'
    skip=site+10+struct.unpack('<b',read(site+9,1))[0]
    assert skip>site+10 and skip<site+80
assert bytes.fromhex('83f805') in read(0x780980,0x20)
assert read(0x840aef,10)==bytes.fromhex('c7870014000001000000')
print('Verified all eight null-weapon crash sites and native unarmed ID/state contracts.')
