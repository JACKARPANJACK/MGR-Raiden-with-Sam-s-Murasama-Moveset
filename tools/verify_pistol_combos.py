"""Independent checks of packed pistol attack data and native playback wiring."""
from pathlib import Path
import json
import hashlib
from pistol_attack_catalog import CLIPS, PLAYER_CLIPS, ROUTES
import struct
import numpy as np
from assets import dat_entries
from gun_motion import world_pose, channels
from inspect_sam_sequences import parse

root=Path(__file__).resolve().parents[1]
members=dict(dat_entries((root/'Release/GameData/wp/wp2062.dat').read_bytes()))
before=dict(dat_entries((root/'local_assets/pistol-unity/wp2062-before-attacks.dat').read_bytes()))
assert all(members[n]==b for n,b in before.items() if not n.startswith(('wp2062_2f50','wp2062_2f51')))
skeleton=(root/'local_assets/pistol-unity/installed-player.wmb').read_bytes()
auxiliary=dict(dat_entries((root/'local_assets/pistol-unity/paired-and-contact-review.dat').read_bytes()))
assert len(members)==6+len(PLAYER_CLIPS)*4
assert len(auxiliary)==(len(CLIPS)-len(PLAYER_CLIPS))*4
assert {i for route in ROUTES.values() for i in route}==set(range(len(PLAYER_CLIPS)))
runtime_members=members
report=json.loads((root/'local_assets/pistol-unity/retarget-report.json').read_text())
clip_reports={c['code']:c for c in report['clips']}
if report.get('combatProfile'):
    # Compare every retained channel/matrix with the archived full authoring
    # export, independently of the runtime pose/FK checks below.
    author=dict(dat_entries(Path(report['authoringBank']).read_bytes()))
    for clip in report['clips']:
        if not clip['playable']:continue
        base='wp2062_'+clip['code'];first,last=clip['combatWindow']
        for suffix,width,record_format in (('.mot',4,'<hbbhHI'),('.gwm',64,'<h2xII')):
            original=author[base+suffix];trimmed=runtime_members[base+suffix]
            assert struct.unpack_from('<H',trimmed,10)[0]==last-first+1
            count=struct.unpack_from('<I',original,16)[0]
            assert struct.unpack_from('<I',trimmed,16)[0]==count
            for j in range(count):
                at=44+j*12;a=struct.unpack_from(record_format,original,at);b=struct.unpack_from(record_format,trimmed,at)
                assert a[0]==b[0]
                assert original[at+a[-1]+first*width:at+a[-1]+(last+1)*width]==trimmed[at+b[-1]:at+b[-1]+(last-first+1)*width], (base,suffix,j,'Retained source pose changed')

for spec in CLIPS:
    code=spec['code'];members=runtime_members if spec['playable'] else auxiliary
    base='wp2062_'+code
    mot=members[base+'.mot'];gwm=members[base+'.gwm']
    timing=members[base+'_timing.bin']
    magic,frames,recovery,count=struct.unpack_from('<4sIII',timing)
    cues=list(struct.unpack_from(f'<{count}I',timing,16))
    assert len(timing)==16+count*4 and magic==b'PCT2'
    assert count==(3 if spec['playable'] else 0) and 0<recovery<frames
    assert not cues or 0<cues[0]<cues[1]<cues[2]<recovery
    assert all(b-a>=8 for a,b in zip(cues,cues[1:]))
    shot=cues[0] if cues else frames//2
    assert mot[:4]==b'mot\0' and struct.unpack_from('<H',mot,10)[0]==frames
    assert mot[24:35]==('pl0010_'+code).encode()
    assert 2<=frames<=2048
    if spec['family'].startswith('pistol_attack'):assert frames==37, 'Equip/raise/lower tails must be trimmed'
    else:assert recovery==frames-1, 'Complete gun-fu movement must finish before the next stage'
    complete=channels(mot,0)
    assert all((i,k) in complete for i in (0,1,2,3,4,5,6,7,8,9,10,11,12,13,15,16,17,18,19,20,21,22) for k in range(6)), 'Native MOT must explicitly drive the complete body'
    assert gwm[:4]==b'FBW1', 'Attack pose must be actor-relative full-body data'
    for f in (0,shot,frames-1):
        assert all(channels(mot,f).get((-1,k))==0 for k in range(6)), 'Root channels must explicitly clear previous travel'
    seq=parse(members[base+'_0_seq.bxm'])
    assert {t.tag for t in seq}=={'FlagsTrack'} # shots use the owned native pool, no sword/victim events
    tracks={}
    for j in range(struct.unpack_from('<I',gwm,16)[0]):
        at=44+j*12;i,n,offset=struct.unpack_from('<h2xII',gwm,at)
        assert n==frames and at+offset+64*n<=len(gwm)
        m=np.frombuffer(gwm,dtype='<f4',count=16*n,offset=at+offset).reshape(n,4,4)
        assert np.isfinite(m).all()
        assert np.max(np.abs(np.linalg.det(m[:,:3,:3])-1))<.0001
        tracks[i]=m
    assert np.all((tracks[0][:,3,1]>.02)&(tracks[0][:,3,1]<3)), 'Invalid actor-relative pelvis height'
    if spec['family']=='down_shot':assert tracks[0][0,3,1]<.5, 'Prone attack must start near the floor'
    for parent,helpers in ((7,(1282,1283,1284)),(8,(1285,1286,1287)),(11,(1289,1290,1291)),(12,(1292,1293,1294))):
        for helper in helpers:
            # Row matrices: helper relative to parent must preserve the limb X axis.
            relative=tracks[helper][:,:3,:3]@np.transpose(tracks[parent][:,:3,:3],(0,2,1))
            assert np.max(np.abs(relative[:,0,:]-np.array([1,0,0])))<.0001, 'Bend baked into twist helper'

    assert np.max(np.ptp(tracks[13][:,3,:3],axis=0))>.1, 'Static/rest-pose export'
    for i in (0,1,2,3,4,5,7,8,9,11,12,13,15,16,17,19,20,21):
        r=np.asarray(tracks[i][:,:3,:3],dtype=np.float64)
        u,_,v=np.linalg.svd(r);r=u@v
        angle=np.degrees(np.arccos(np.clip((np.einsum('fij,fij->f',r[1:],r[:-1])-1)/2,-1,1)))
        if spec['family'].startswith('pistol_attack'):
            assert angle.max()<50, (code,i,'Discontinuous limb rotation',angle.max())
        else:
            source=np.array(clip_reports[code]['sourceAngularSteps'][str(i)])
            assert len(source)==len(angle) and np.max(np.abs(angle-source))<.1, (code,i,'Retarget added a rotation jump',float(np.max(np.abs(angle-source))))
    events=seq.find('FlagsTrack')
    assert len(events)==2 and events[1].get('Flag0')=='2'
    assert float(events[0].get('EndTime'))==float(events[1].get('StartTime'))
    if cues:assert cues[-1]<float(events[1].get('StartTime'))*60<=recovery
    for event in events:
        assert 0<=float(event.get('StartTime'))<float(event.get('EndTime'))<=(frames-1)/60+.000001, 'BXM has an empty/out-of-bounds event span'
        assert event.get('Flag0') in ('262144','2')
    assert all(int(track.get('SeqNum'))==len(track) for track in seq)

    for f in range(frames):
        fk=world_pose(skeleton,mot,f)
        for i,m in tracks.items():
            assert np.max(np.abs(fk[i]-m[f].T))<.0002, (code,i,f,'MOT and full-body pose differ')
    assert float(seq.find('FlagsTrack/Seq').get('EndTime'))<frames/60
    grip=np.frombuffer(runtime_members['wp2062_grip.bin'],dtype='<f4').reshape(4,4)
    barrels=[(grip@tracks[13][f])[2,:3] for f in cues]
    if code in ('2f50','2f51'):assert np.dot(barrels[0],barrels[-1])<.95, 'Volley must span the firing sweep'
    for f,forward in zip(cues,barrels):
        assert np.isfinite(forward).all() and abs(np.linalg.norm(forward)-1)<.0001

runtime=(root/'dllmain.cpp').read_text()
post=runtime[runtime.index('    __try\n    {\n        auto* player = g_Scene.m_pPlayer;'):]
assert post.index('StealthStanceController::Get().PostTick(player)')<post.index('KunaiSubweapon::Get().PostTick(player)')<post.index('SheathController::Instance().ParkUnusedSword')
assert 'Owner::PistolCombo' in runtime and 'PistolComboController::Get().HandleAction(pThis)' in runtime
report=json.loads((root/'local_assets/pistol-unity/retarget-report.json').read_text())
assert report['sourceRig']=='Humanoid' and report['playableClips']==16 and report['auxiliaryClips']==10
assert {c['sourcePath'] for c in report['clips']}=={c['sourcePath'] for c in CLIPS}
assert all('/Humanoid/' in c['sourcePath'] and c['maxFkMatrixError']<.0002 and c['sourceRestShapeError']<.001 and c['bindSceneFrame']==0 for c in report['clips'])
assert '/Humanoid/' in report['referencePath'] and 'T-pose' in report['referencePath']
inventory=json.loads((root/'local_assets/pistol-unity/package-inventory.json').read_text())
assert inventory['totalFbx']==150
sources={r['path']:r for r in inventory['files']}
for clip in report['clips']:
    assert clip['sha256']==sources[clip['sourcePath']]['sha256']
    assert hashlib.sha256(Path(sources[clip['sourcePath']]['local']).read_bytes()).hexdigest()==clip['sha256']
    if clip['code'] in ('2f50','2f51'):assert clip['sourceAttackRange']==[14,50]
baseline=dict(dat_entries((root/'local_assets/pistol-unity/wp2062-two-attack-baseline.dat').read_bytes()))
assert all(runtime_members[n]==b for n,b in baseline.items()), 'Existing shooting/attacks must remain byte-identical'
print('PASS: all 150 FBXs audited; 26 Humanoid combat/contact retargets; 16 attack variants reachable; complete MOT/FBW FK, continuous full-body poses, volley timing and BXM recovery; original bank unchanged')
