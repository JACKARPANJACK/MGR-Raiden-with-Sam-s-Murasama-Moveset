"""Verify the retail animation maintenance and direct-playback entry points."""
from pathlib import Path
import struct

root=Path(__file__).resolve().parents[1]
game=Path('C:/Program Files (x86)/Steam/steamapps/common/METAL GEAR RISING REVENGEANCE')
exe=(game/'METAL GEAR RISING REVENGEANCE.exe').read_bytes()
pe=struct.unpack_from('<I',exe,60)[0]
count=struct.unpack_from('<H',exe,pe+6)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]


def read(rva,size):
    for j in range(count):
        _,va,n,offset=struct.unpack_from('<4I',exe,table+j*40+8)
        if va<=rva and rva+size<=va+n:return exe[offset+rva-va:offset+rva-va+size]
    raise ValueError(hex(rva))


assert read(0x794790,4)==bytes.fromhex('568bf157')
assert read(0x7947FD,2)==bytes.fromhex('8bce')
assert read(0x7947FF,1)==b'\xe8'
assert 0x794804+struct.unpack('<i',read(0x794800,4))[0]==0x6A4AC0
assert read(0x6A4AC0,1)==b'\xe9'
assert 0x6A4AC5+struct.unpack('<i',read(0x6A4AC1,4))[0]==0x6A36F0
# Common maintenance reads the animation node's current time and stores it
# in the slot, then updates motion/sequence state. It is not weapon-only.
assert read(0x6A3765,1)==b'\x57'
assert read(0x6A3771,6)==bytes.fromhex('8d5608d95e28')
# setDirectAnimation resets all three native animation playback multipliers.
assert read(0x69F093,20)==bytes.fromhex('d9e8d996e4000000d996e8000000d99eec000000')[:20]
controller=(root/'PistolComboController.h').read_text()
assert 'player->setDirectAnimation(clip.motion.data(),clip.sequence.data()' in controller
active=controller[controller.index('        if(active) {'):]
assert active.index('shared::base+0x794790')<active.index('player->getRemainingAnimationFrames(0)')
assert 'Stop(player);return false;' in active
print('PASS: retail common animation maintenance, slot-time update, direct playback multipliers and stalled-action release')
