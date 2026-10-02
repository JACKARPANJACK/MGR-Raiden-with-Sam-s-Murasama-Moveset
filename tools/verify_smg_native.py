"""Verify enemy SMG ABI evidence against a locally owned Win32 game EXE."""
from pathlib import Path
import struct
import sys
d=Path(sys.argv[1]).read_bytes()
pe=struct.unpack_from('<I',d,0x3c)[0]
assert d[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',d,pe+4)[0]==0x14c
opt=pe+24;size=struct.unpack_from('<H',d,pe+20)[0]
base=struct.unpack_from('<I',d,opt+28)[0]
sections=[]
for i in range(struct.unpack_from('<H',d,pe+6)[0]):
    o=opt+size+40*i
    _,va,length,raw=struct.unpack_from('<IIII',d,o+8)
    sections.append((va,length,raw))
def read(a,n):
    for va,length,raw in sections:
        if va<=a and a+n<=va+length:return d[raw+a-va:raw+a-va+n]
    raise AssertionError(hex(a))
assert read(0x7060c7,5)==bytes.fromhex('6820000300') # wp0020 gun
assert read(0x7060b2,5)==bytes.fromhex('6801070000') # right hand marker
assert read(0x71cea8,8)==bytes.fromhex('c744247446000200') # em0046 bullet
assert read(0x71ceb0,11)==bytes.fromhex('c784248001000058000000')
assert read(0x71d0a5,11)==bytes.fromhex('c7842480000000df000000') # bullet attack
assert read(0x71d0de,5)==bytes.fromhex('834c247004')
for site,target in ((0x71ce8a,0x105d0),(0x71ce96,0x10710),(0x71ce9f,0x1cf30),
                    (0x71d111,0x16e30),(0x71d122,0x6d3be0)):
    b=read(site,5)
    assert b[0]==0xe8 and site+5+struct.unpack_from('<i',b,1)[0]==target
assert abs(struct.unpack('<f',read(0x1641b88-base,4))[0]-250)<0.001
print('Verified wp0020, right-hand mount, enemy projectile descriptor, attack DF, constructors, trajectory and factory.')
assert read(0x1cf32,7)==bytes.fromhex('c7410400b00300')
print('Verified native BulletBase constructor defaults to the generic wpb000 bullet prefab (0x3B000).')
for rva,expected in ((0x5930a0,'5355568bf1'),(0x5928a0,'56578b3d'),
    (0x5a48f0,'81ecd0000000'),(0x592b0e,'83f809774dff2485'),
    (0x5b43ee,'83f809774aff2485'),(0x5a49a0,'c7442440'),(0x593a94,'8b048d')):
    assert read(rva,len(bytes.fromhex(expected)))==bytes.fromhex(expected),hex(rva)
for site,target in ((0x592b16,0x592ce8),(0x5b43f6,0x5b4654),
                    (0x5a49a4,0x14b5778),(0x593a97,0x12567e4)):
    assert struct.unpack('<I',read(site,4))[0]==base+target,hex(site)
assert struct.unpack('<10I',read(0x592ce8,40))==tuple(base+x for x in
    (0x592b1a,0x592b21,0x592b28,0x592b2f,0x592b36,0x592b3d,0x592b44,0x592b4b,0x592b52,0x592b59))
assert struct.unpack('<10I',read(0x5b4654,40))==tuple(base+x for x in
    (0x5b43fa,0x5b4401,0x5b4408,0x5b440c,0x5b4413,0x5b441a,0x5b4421,0x5b4428,0x5b442f,0x5b4436))
assert 0x3f0-0x3c4==0x464-0x438==0x49c-0x470==11*4
assert read(0x5924f6,2)==bytes.fromhex('6a2c')
print('Verified native secondary menu build, cursor, eleven-cell capacity, ten stock slot mappings, drawing and sound tables.')
for rva,expected in ((0x5a4786,'8d1c9d'),(0x8b1cd0,'83ec088b4120'),
                     (0x8b1bf0,'8b0185c0'),(0x6831e0,'558bec83e4f0'),
                     (0x682610,'568bf1'),(0x683330,'558bec83e4f0')):
    assert read(rva,len(bytes.fromhex(expected)))==bytes.fromhex(expected),hex(rva)
assert struct.unpack('<I',read(0x5a4789,4))[0]==base+0x12b5470
assert read(0x71f878,2)==bytes.fromhex('6a16')
for site,target in ((0x71f881,0x682610),(0x71f8c1,0x683270),
                    (0x712a5d,0x682640),(0x712a6e,0x683330)):
    call=read(site,5)
    assert call[0]==0xe8 and site+5+struct.unpack_from('<i',call,1)[0]==target
assert read(0x712d9f,4)==bytes.fromhex('6a006a1f') # gun ready map31
assert read(0x712acb,4)==bytes.fromhex('6a006a21') # raise map33
assert read(0x712ec0,4)==bytes.fromhex('6a006a22') # recoil map34
print('Verified private native menu message path and standalone enemy spine aiming controller/ready-raise-recoil sequence.')
for rva,expected in ((0x7a40dc,'c78424b400000002100300'),
                     (0x7a4186,'c78424b4000000e1100300'),
                     (0x7a40e7,'c78424c001000027000000'),
                     (0x7a4191,'c78424c001000048000000'),
                     (0x7a4261,'c78424c000000055000000')):
    assert read(rva,len(bytes.fromhex(expected)))==bytes.fromhex(expected),hex(rva)
for site,target in ((0x7a40ae,0x105d0),(0x7a40ba,0x10710),(0x7a40c6,0x1cf30),
                    (0x7a42da,0x16e30),(0x7a4336,0x3fed0),(0x7a4381,0x6d3be0)):
    b=read(site,5)
    assert b[0]==0xe8 and site+5+struct.unpack_from('<i',b,1)[0]==target,hex(site)
assert abs(struct.unpack('<f',read(0x123cf88,4))[0]-0.6)<0.001
assert abs(struct.unpack('<f',read(0x123cf90,4))[0]-200)<0.001
print('Verified native RPG/Stinger objects, projectile types, battle attack, trajectory, guided-target setup and speed.')
asset_root=Path(__file__).resolve().parent.parent/'local_assets/data000/pl/pl0010.dat.unpacked'
if asset_root.exists():
    for code,frames in (('2570',146),('2571',176),('2574',181),('2575',168),('2576',166),('2577',201),('2578',281)):
        blob=(asset_root/f'pl0010_{code}.mot').read_bytes()
        assert blob[:4]==b'mot\0' and struct.unpack_from('<H',blob,10)[0]==frames,code
    print('Verified all seven supplied Raiden combo motion files and native frame counts.')
gun_root=Path(__file__).resolve().parent.parent/'local_assets/smg/wp0020'
if gun_root.exists():
    from merge_sam_bodies import bone_index
    from assets import dat_entries
    from inspect_sam_sequences import parse
    assert bone_index((gun_root/'wp0020.wmb').read_bytes(),0x300)==2
    effects=dict(dat_entries((gun_root/'wp0020.eff').read_bytes()))
    assert 0 in {int(entry.get('id')) for entry in parse(effects['header.bxm']).find('est')}
    print('Verified wp0020 muzzle bone 0x300 and native muzzle effect 0.')
