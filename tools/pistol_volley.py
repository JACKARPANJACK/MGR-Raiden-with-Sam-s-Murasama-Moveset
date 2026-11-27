"""Author movement-timed pistol volleys without changing retargeted motions."""
import json
from pathlib import Path
import struct


def timing(frames, first):
    spacing=min(18,(frames-first-3)//2)
    if spacing<8:raise ValueError('Attack is too short for three separated shots')
    cues=[first+i*spacing for i in range(3)]
    recovery=min(frames-1,cues[-1]+8)
    if not 0<first<cues[-1]<recovery<frames:
        raise ValueError('Pistol motion is too short for its three-round volley')
    return struct.pack('<4sIII3I',b'PCT2',frames,recovery,len(cues),*cues),cues,recovery


def main():
    from assets import dat_entries
    from merge_sam_bodies import pack_members
    from inspect_sam_sequences import parse
    from build_dual_blade_animations import encode_bxm
    root=Path(__file__).resolve().parents[1]
    bank=root/'Release/GameData/wp/wp2062.dat'
    original=dict(dat_entries(bank.read_bytes()));members=dict(original)
    report_path=root/'local_assets/pistol-unity/retarget-report.json'
    report=json.loads(report_path.read_text())
    for code in ('2f50','2f51'):
        base='wp2062_'+code
        old=members[base+'_timing.bin']
        if old[:4]==b'PCT1':first=struct.unpack_from('<I',old,8)[0]
        elif old[:4]==b'PCT2':first=struct.unpack_from('<I',old,16)[0]
        else:raise ValueError('Unknown pistol timing format')
        frames=struct.unpack_from('<H',members[base+'.mot'],10)[0]
        data,cues,recovery=timing(frames,first)
        seq=parse(members[base+'_0_seq.bxm']);flags=seq.find('FlagsTrack')
        assert len(flags)==2
        unlock=max(cues[-1]+1,recovery-2)
        flags[0].set('EndTime',f'{unlock/60:.6f}')
        flags[1].set('StartTime',f'{unlock/60:.6f}')
        members[base+'_0_seq.bxm']=encode_bxm(seq)
        members[base+'_timing.bin']=data
        for clip in report['clips']:
            if clip['code']==code:clip.update(volleyFrames=cues,recoveryFrame=recovery)
        print(f'{code}: volley frames {cues}; recovery {recovery}')
    for name,b in original.items():
        if name.endswith(('.mot','.gwm','.bin')) and not name.endswith('_timing.bin'):assert members[name]==b
    packed=pack_members(members);assert dict(dat_entries(packed))==members
    bank.write_bytes(packed);report_path.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
