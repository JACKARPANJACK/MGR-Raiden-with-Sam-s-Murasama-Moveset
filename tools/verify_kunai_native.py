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
call(0x99fac1,0x9e4dc0) # camera unprojection through native viewport and view matrix
assert read(0x99fab4,6)==bytes.fromhex('8d81b0000000')
assert read(0xb98a90,6)==bytes.fromhex('a1dc06f201c3') # native viewport width getter
assert read(0xb98aa0,6)==bytes.fromhex('a1e006f201c3') # native viewport height getter
call(0x7a44ca,0x67c8a0) # behavior lookup: must never be patched as "ammo"
call(0x7e91f8,0x68c5f0) # optional secondary attachment null-handle crash site
call(0x7954d3,0x78a740) # original Raiden sword placement entry
call(0x463db3,0x78a740) # original Sam sword placement entry
call(0x795568,0x612210) # animated 0x720 marker lookup, not a bone array index
call(0x463dfa,0x612210)
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
assert bytes.fromhex('0d00880000') in read(0x7a4696,0x10) # OR EAX, native friendly collision filter
print('Verified all eight null-weapon crash sites and native unarmed ID/state contracts.')
call(0x7bd989,0x67c8a0) # observed scene-reload crash: null sword entity
call(0x7bd990,0x611c60) # native sword setup follows behavior lookup
assert read(0x7bd95e,4)==bytes.fromhex('85ff741d') # factory has a missing-entity branch
assert read(0x7bd7ce,5)==bytes.fromhex('0510070000') # preserve prologue +1 sheath handling
assert read(0xa00c50,3)==bytes.fromhex('83ec14') # native EFF/EFT registration, cdecl
call(0xa00c69,0x5f9420)
call(0xa00c94,0x9e3d30) # EFF lookup in the supplied archive
call(0xa00d33,0xb4c130) # registers actual data pointers under the native bank ID
assert read(0x12ca67c,4)==b'eff\0' and read(0x12ca678,4)==b'eft\0'
assert read(0xaaa9b0,4)==bytes.fromhex('568bf18b') # cEspControler native teardown
assert read(0x54dfd0,4)==bytes.fromhex('568bf157') # native item definition lookup
assert read(0x77f840,1)==b'\xe8' # Raiden equipped-item alias helper
assert read(0x551c80,4)==bytes.fromhex('53558b6c') # native possession factory
assert struct.pack('<I',0x154b4aab) in read(0x551ced,0x20) # dispatches actual heatknife class
knife_table=struct.unpack('<22I',read(0x1250094,88))
assert knife_table[5]-base==0x549790 and knife_table[9]-base==0x54d8d0
assert knife_table[12]-base==0x5497a0 # use calls noUse then spend, not unlimited plugin ammo
assert struct.unpack('<II',read(0x1256760,8))==(0x154b4aab,9) # stock menu's knife entry
assert read(0x592b59,5)==bytes.fromhex('be0a000000') # menu maps knife entry 9 -> equipped slot 10
assert read(0x54d8d0,3)==bytes.fromhex('ff4954') # native ammo spend decrements +54
print('Verified native knife factory, menu entry, slot 10 mapping, capacity getter and ammo consumption.')
assert read(0x6d054b,6)==bytes.fromhex('8d9e20090000') # BulletBase launch velocity
assert read(0x6d2473,6)==bytes.fromhex('d98620090000') # native movement uses velocity.x
assert read(0x6d247f,6)==bytes.fromhex('d98624090000') # native movement uses velocity.y
assert read(0x6d2576,12)==bytes.fromhex('d98620090000d99e70110000') # rigid-body transfer
assert read(0x7a4669,11)==bytes.fromhex('c78424c400000011100300') # native grenade object
assert read(0x7a465e,11)==bytes.fromhex('c78424d401000011000000') # grenade update type 0x11
assert read(0x7a4674,11)==bytes.fromhex('c78424d001000026000000') # grenade behavior tag
assert read(0x7a46c9,11)==bytes.fromhex('c78424d000000057000000') # grenade attack 0x57
print('Verified native projectile velocity integration and grenade payload metadata for charged kunai.')
call(0xaaa9f6,0xaaa8c0) # kill linked particle units before destroying controller
call(0xaaa9fd,0xaaa840) # detach unit backlinks before releasing controller storage
call(0x16e3b9,0x105d0)
call(0x16e3c5,0x10710)
call(0x16e3ce,0x1cf30)
call(0x16e51e,0x5f8b60) # category pointer getter, not value getter 5F8B40
call(0x16e5d6,0x16e30)
print('Verified scene sword failure, untouched native sheath operand, effect registration ABI, controller teardown and Wolf trajectory initialization.')
call(0xa00d66,0x5f9420)
call(0xa00d81,0x9e3d30)
call(0xa00dcd,0xb4bc40) # balances the native EFF registration reference
assert read(0xb4c284,3)==bytes.fromhex('ff463c') # duplicate registrations acquire a ref
assert struct.pack('<I',base+0x1778858) in read(0x5dca60,0x60)
assert struct.pack('<I',base+0x17781b8) in read(0x5cbde0,0x30)
assert struct.pack('<I',base+0x19bd180) in read(0x876d60,0x50)
assert struct.pack('<I',base+0x19bd184) in read(0x876d60,0x50)
print('Verified effect-bank reference balance and DLC sound/effect/bullet selector mutations requiring scene restoration.')
