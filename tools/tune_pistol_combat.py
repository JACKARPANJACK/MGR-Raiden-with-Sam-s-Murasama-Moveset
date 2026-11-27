"""Make compact combat pairs from the verified Humanoid exports.

Preserve every retained bone pose exactly; slice MOT, FBW and cue timing
together. The complete source exports remain available in an immutable backup.
"""
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
from assets import dat_entries
from merge_sam_bodies import pack_members
from build_dual_blade_animations import encode_bxm
from pistol_attack_catalog import write_header

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'local_assets/pistol-unity'


def slice_mot(data,first,last):
    count=struct.unpack_from('<I',data,16)[0];frames=last-first+1
    result=bytearray(data[:44]);struct.pack_into('<H',result,10,frames)
    result.extend(bytes(12*(count+1)))
    for j in range(count):
        at=44+12*j;bone,prop,kind,n,unused,relative=struct.unpack_from('<hbbhHI',data,at)
        assert kind==1 and last<n
        struct.pack_into('<hbbhHI',result,at,bone,prop,kind,frames,unused,len(result)-at)
        result.extend(data[at+relative+first*4:at+relative+(last+1)*4])
    struct.pack_into('<h',result,44+12*count,32767)
    return bytes(result)


def slice_fbw(data,first,last):
    count=struct.unpack_from('<I',data,16)[0];frames=last-first+1
    result=bytearray(data[:44]);struct.pack_into('<H',result,10,frames)
    result.extend(bytes(12*count))
    for j in range(count):
        at=44+12*j;bone,n,relative=struct.unpack_from('<h2xII',data,at)
        assert last<n
        struct.pack_into('<h2xII',result,at,bone,frames,len(result)-at)
        result.extend(data[at+relative+first*64:at+relative+(last+1)*64])
    return bytes(result)


def main():
    bank=ROOT/'Release/GameData/wp/wp2062.dat';report_path=OUT/'retarget-report.json'
    report=json.loads(report_path.read_text())
    if report.get('combatProfile'):
        bank_bytes=Path(report['authoringBank']).read_bytes()
        report=json.loads(Path(report['authoringReport']).read_text())
    else:bank_bytes=bank.read_bytes()
    digest=hashlib.sha256(bank_bytes).hexdigest()[:12]
    author_bank=OUT/f'authoring-{digest}.dat';author_report=OUT/f'authoring-{digest}.json'
    if not author_bank.exists():author_bank.write_bytes(bank_bytes)
    if not author_report.exists():author_report.write_text(json.dumps(report,indent=2)+'\n')
    original=dict(dat_entries(bank_bytes));members=dict(original)
    for c in report['clips']:
        if not c['playable']:continue
        base='wp2062_'+c['code'];first=0;last=c['frames']-1
        if not c['family'].startswith('pistol_attack'):
            first=max(0,c['volleyFrames'][0]-8)
            last=min(last,c['volleyFrames'][-1]+10)
            members[base+'.mot']=slice_mot(original[base+'.mot'],first,last)
            members[base+'.gwm']=slice_fbw(original[base+'.gwm'],first,last)
            cues=[f-first for f in c['volleyFrames']];frames=last-first+1;recovery=frames-1
            members[base+'_timing.bin']=struct.pack('<4sIII3I',b'PCT2',frames,recovery,3,*cues)
            seq=ET.Element('SeqRoot');track=ET.SubElement(seq,'FlagsTrack',SeqNum='2')
            unlock=max(cues[-1]+1,recovery-2)
            for a,b,flag in ((0,unlock,262144),(unlock,recovery,2)):
                ET.SubElement(track,'Seq',LayerFlag='4294967295',StartTime=f'{a/60:.6f}',EndTime=f'{b/60:.6f}',Config='0',Flag0=str(flag),Flag1='0',FreeArg='0 0 0 0')
            members[base+'_0_seq.bxm']=encode_bxm(seq)
            c['sourceAngularSteps']={i:v[first:last] for i,v in c['sourceAngularSteps'].items()}
            c['sourceAttackRange']=[c['sourceAttackRange'][0]+first,c['sourceAttackRange'][0]+last]
            c.update(frames=frames,volleyFrames=cues,shotFrame=cues[0],recoveryFrame=recovery)
        c['combatWindow']=[first,last]
        print(c['code'],c['family'],c['frames'],'frames',c['volleyFrames'])
    # Regular ready/aim/grip and the two proven basic attacks remain untouched.
    for name,value in original.items():
        if name.startswith(('wp2062_2f4','wp2062_2f50','wp2062_2f51')) or name=='wp2062_grip.bin':assert members[name]==value
    packed=pack_members(members);assert dict(dat_entries(packed))==members
    bank.write_bytes(packed)
    report.update(combatProfile='responsive-gunfu-v1',authoringBank=str(author_bank),authoringReport=str(author_report),
                  sourceContinuityFrames=sum(c['frames'] for c in report['clips']))
    report_path.write_text(json.dumps(report,indent=2)+'\n');write_header(ROOT)


if __name__=='__main__':main()
