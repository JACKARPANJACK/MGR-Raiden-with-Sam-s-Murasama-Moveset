"""Verify Sai dispatch/weapon-bank ABI and unused animation pairs locally."""
import struct
from pathlib import Path

def dat_entries(data):
    assert data[:4]==b'DAT\0'
    count,offsets,_,names,sizes,_=struct.unpack_from('<6I',data,4)
    width=struct.unpack_from('<I',data,names)[0]
    assert 0<count<10000 and 0<width<4096
    result=[]
    for index in range(count):
        at=names+4+index*width
        name=data[at:at+width].split(b'\0')[0].decode('ascii')
        start=struct.unpack_from('<I',data,offsets+index*4)[0]
        length=struct.unpack_from('<I',data,sizes+index*4)[0]
        assert start+length<=len(data)
        result.append((name,data[start:start+length]))
    return result

ROOT=Path(__file__).resolve().parents[1]
GAME=Path(r'C:/Program Files (x86)/Steam/steamapps/common/METAL GEAR RISING REVENGEANCE')
blob=(GAME/'METAL GEAR RISING REVENGEANCE.exe').read_bytes()
pe=struct.unpack_from('<I',blob,0x3c)[0]
assert blob[pe:pe+4]==b'PE\0\0'
assert struct.unpack_from('<H',blob,pe+4)[0]==0x14c
base=struct.unpack_from('<I',blob,pe+24+28)[0]
count=struct.unpack_from('<H',blob,pe+6)[0]
sections=pe+24+struct.unpack_from('<H',blob,pe+20)[0]
def read(rva,size):
    for i in range(count):
        _,start,length,raw=struct.unpack_from('<IIII',blob,sections+i*40+8)
        if start<=rva and rva+size<=start+length:
            return blob[raw+rva-start:raw+rva-start+size]
    raise AssertionError(hex(rva))
def expect(rva,value):
    assert read(rva,len(value))==value, f'Sai ABI mismatch at {rva:x}'
expect(0x805000,bytes.fromhex('8b81180600003d41010000'))
for state,handler in {0x9C:0x7c5880,0x9D:0x7db200,0x9E:0x7c61c0,
                      0x9F:0x7a1c30,0xA0:0x7a1d80,0xA1:0x7c6760,0xA2:0x7a1f00,
                      0xA3:0x7a1c30,0xA4:0x7a1d80,0xA5:0x7a1d80}.items():
    stub=struct.unpack('<I',read(0x8054d4+state*4,4))[0]-base
    ins=read(stub,5)
    assert ins[0]==0xe9 and stub+5+struct.unpack_from('<i',ins,1)[0]==handler
# New hook is Behavior::setAnimationByData: eight stack arguments, cFmerge MOT
# index lookup, ret 0x20. Recovery A4/A5 use indices 26/27, not Spin/Somersault.
expect(0x69f3c0,bytes.fromhex('8b44240881ec90000000'))
expect(0x69f4bc,bytes.fromhex('c22000'))
expect(0x7a1dd4,bytes.fromhex('bb18000000'))
expect(0x7a1de0,bytes.fromhex('bb1a000000'))
expect(0x7a1dec,bytes.fromhex('bb1b000000'))
# Spin's A1 handler requests MOT index 10 (2b20), not A2's index 19
# (2b70 forward flip). Both action entry and motion must agree.
expect(0x7c68a2,bytes.fromhex('6a0a'))
expect(0x7a2044,bytes.fromhex('6a13'))
# Native 9E integrates floating-point vertical velocity at +894 unless the
# repeat-air-attack suppression at +948 is set, and lands via MOT index 15.
expect(0x7c634c,bytes.fromhex('d98694080000'))
expect(0x7c66c4,bytes.fromhex('6a0f'))
expect(0x7c66d4,bytes.fromhex('83a654260000fb'))
entries=dat_entries((ROOT/'local_assets/data000/wp/wp2030.dat').read_bytes())
lookup=dict(entries)
motions=[name for name,_ in entries if name.endswith('.mot')]
for index,code in {0:'2b00',1:'2b01',2:'2b02',5:'2b10',6:'2b11',7:'2b12',
                   10:'2b20',11:'2b21',12:'2b22',13:'2b23',
                   14:'2b50',19:'2b70',20:'2b71',21:'2b72',22:'2b75',27:'2b7a'}.items():
    stem=f'pl0010_{code}'
    assert motions[index]==stem+'.mot'
    assert lookup[stem+'.mot'][:4]==b'mot\0'
    assert lookup[stem+'_0_seq.bxm'][:4] in (b'XML\0',b'BXM\0')
print('Sai native dispatch, weapon motion indices, unused animation pairs and hook ABI verified.')
