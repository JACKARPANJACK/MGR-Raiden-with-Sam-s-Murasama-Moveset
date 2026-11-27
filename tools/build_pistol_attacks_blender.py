"""Retarget the user's Unity FBX pistol attacks and extend wp2062.dat.

Blender --background --factory-startup --python this_file -- --repo PATH --package PATH
Package contents are animation data, never build instructions.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import bpy
import numpy as np
from mathutils import Matrix, Vector


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--package', type=Path, required=True)
    p.add_argument('--only',nargs='+',help='Rebuild selected families while preserving all other validated clips')
    p.add_argument('--skeleton', type=Path, help='Installed body WMB used for bind calibration')
    args = p.parse_args(sys.argv[sys.argv.index('--')+1:])
    repo = args.repo.resolve()
    sys.path.insert(0, str(repo/'tools'))
    from assets import dat_entries
    from merge_sam_bodies import bones, pack_members
    from gun_motion import world_pose
    from build_stealth_animations_blender import euler_xyz
    from pistol_retarget import Retarget
    from pistol_volley import timing
    from pistol_attack_catalog import CLIPS, write_header
    write_header(repo)
    from build_dual_blade_animations import encode_bxm
    output = repo/'local_assets/pistol-unity'
    output.mkdir(parents=True, exist_ok=True)
    bank = repo/'Release/GameData/wp/wp2062.dat'
    prior=json.loads((output/'retarget-report.json').read_text()) if args.only else {}
    # Partial authoring rebuilds must start from the complete source export,
    # otherwise tuning would trim already compacted, unselected clips twice.
    authoring=bank.read_bytes()
    if prior.get('combatProfile'):
        authoring=Path(prior['authoringBank']).read_bytes()
        prior=json.loads(Path(prior['authoringReport']).read_text())
    original = dict(dat_entries(authoring))
    members = dict(original)
    auxiliary = dict(dat_entries((output/'paired-and-contact-review.dat').read_bytes())) if args.only else {}
    baseline=output/'wp2062-two-attack-baseline.dat'
    if not baseline.exists():baseline.write_bytes(bank.read_bytes())
    skeleton_path=args.skeleton or repo/'local_assets/pistol-unity/installed-player.wmb'
    if not args.skeleton:
        installed=Path('C:/Program Files (x86)/Steam/steamapps/common/METAL GEAR RISING REVENGEANCE/mods/RaidenMoveset/pl/pl1300.dat')
        if installed.exists():skeleton_path.write_bytes(dict(dat_entries(installed.read_bytes()))['pl1300.wmb'])
    if not skeleton_path.exists():raise FileNotFoundError('Extract the installed player WMB before retargeting')
    skeleton = skeleton_path.read_bytes()
    records = bones(skeleton)
    ids = [struct.unpack_from('<h', b)[0] for b in records]
    parents = {i: ids[struct.unpack_from('<h', b, 4)[0]] if struct.unpack_from('<h', b, 4)[0]>=0 else -1 for i,b in zip(ids,records)}
    bind = {i: Vector(struct.unpack_from('<3f', b, 8)) for i,b in zip(ids,records)}
    grip_pose={}
    ready=original['wp2062_2f40.gwm']
    for j in range(struct.unpack_from('<I',ready,16)[0]):
        at=44+j*12;i,n,offset=struct.unpack_from('<h2xII',ready,at)
        grip_pose[i]=Matrix(np.frombuffer(ready,dtype='<f4',count=16,offset=at+offset+12*64).reshape(4,4).T.tolist())
    mapping = {0:'Pelvis',1:'Spine',2:'Spine1',3:'Spine2',4:'Neck',5:'Head',
               6:'L Clavicle',7:'L UpperArm',8:'L Forearm',9:'L Hand',
               10:'R Clavicle',11:'R UpperArm',12:'R Forearm',13:'R Hand',
               15:'L Thigh',16:'L Calf',17:'L Foot',18:'L Toe0',
               19:'R Thigh',20:'R Calf',21:'R Foot',22:'R Toe0'}
    for hand,base in (('L',256),('R',512)):
        for j in range(3):mapping[base+j]=f'{hand} Finger0'+('' if j==0 else str(j))
        for finger,offset in ((1,4),(2,8),(3,12),(4,16)):
            for j in range(3):mapping[base+offset+j]=f'{hand} Finger{finger}'+('' if j==0 else str(j))
    selected=[c for c in CLIPS if not args.only or c['family'] in args.only]
    if args.only and not selected:raise ValueError('Unknown attack family')
    prior=json.loads((output/'retarget-report.json').read_text()) if args.only else {}
    if prior:assert prior['skeletonSha256']==hashlib.sha256(skeleton).hexdigest()
    reports = [c for c in prior.get('clips',[]) if c['code'] not in {s['code'] for s in selected}]
    preview = prior.get('preview',{})
    inventory=json.loads((output/'package-inventory.json').read_text())
    sources={r['path']:r for r in inventory['files']}
    with args.package.open('rb') as package_file:
        package_sha=hashlib.file_digest(package_file,'sha256').hexdigest()
    assert inventory['packageSha256']==package_sha, 'Re-audit the package before using its extracted FBXs'
    reference_record=next(r for r in inventory['files'] if r['rigType']=='humanoid' and r['family'].lower().startswith('t-pose'))
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.context.scene.frame_set(0)
    bpy.ops.import_scene.fbx(filepath=reference_record['local'],use_image_search=False)
    reference_rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    bpy.context.scene.frame_set(0);bpy.context.view_layer.update()
    reference_solver=Retarget(reference_rig,mapping,bind,parents,ids,grip_pose)
    for index, spec in enumerate(selected):
        source=spec['source'];family=spec['family']
        record=sources[spec['sourcePath']]
        assert record['rigType']=='humanoid'
        fbx=Path(record['local']).read_bytes()
        assert hashlib.sha256(fbx).hexdigest()==record['sha256']
        print('RETARGET',spec['code'],spec['sourcePath'],flush=True)
        path = output/(source+'.fbx')
        path.write_bytes(fbx)
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete(use_global=False)
        # The armature object itself carries animated root transforms. A stale
        # scene frame contaminates bind calibration with the previous action.
        bpy.context.scene.frame_set(0)
        bpy.ops.import_scene.fbx(filepath=str(path),use_image_search=False)
        bpy.context.scene.frame_set(0);bpy.context.view_layer.update()
        rig = next(o for o in bpy.context.scene.objects if o.type=='ARMATURE' and o.animation_data and o.animation_data.action)
        # Only the armature is sampled. Evaluating the donor preview mesh
        # adds expensive skin deformation without contributing any motion.
        for obj in list(bpy.context.scene.objects):
            if obj.type in ('MESH','CAMERA','LIGHT'):bpy.data.objects.remove(obj,do_unlink=True)
        action = rig.animation_data.action
        start,end = action.frame_range
        fps = bpy.context.scene.render.fps / bpy.context.scene.render.fps_base
        source_frames = round((end-start)*60/fps)+1
        if not 2<=source_frames<=2048: raise ValueError('Invalid duration')
        standing=family.startswith('pistol_attack')
        solver=Retarget(rig,mapping,bind,parents,ids,grip_pose,standing_shot=standing,
            support_grip=standing,weapon_grip=spec['playable'])
        rest_error=max(((solver.restpos[i]-solver.restpos[0])-
            (reference_solver.restpos[i]-reference_solver.restpos[0])).length for i in mapping if i<256)
        if rest_error>.001:raise ValueError(f'Humanoid bind reference mismatch: {source}: {rest_error}')
        if abs(solver.restpos[0].y-reference_solver.restpos[0].y)>.001:
            raise ValueError('Animated root entered the bind-pose calibration')
        samples=[];source_rotations=[]
        for frame in range(source_frames):
            t=start+frame*fps/60
            bpy.context.scene.frame_set(int(t),subframe=t-int(t))
            evaluated=rig.evaluated_get(bpy.context.evaluated_depsgraph_get())
            samples.append(solver.sample(evaluated))
            source_rotations.append({i:np.array((evaluated.matrix_world@evaluated.pose.bones['Bip001 '+mapping[i]].matrix).to_quaternion().to_matrix())
                for i in (0,1,2,3,4,5,7,8,9,11,12,13,15,16,17,19,20,21)})
        if max((p[13].translation-samples[0][0][13].translation).length for p,_ in samples)<.02:
            raise ValueError('FBX evaluation returned a static wrist instead of an attack')
        attack_start=0;attack_end=source_frames-1
        if standing:
            # Preserve the proven attack-only trims for both source variants.
            window=range(max(1,int(source_frames*.15)),int(source_frames*.55))
            extension=max((samples[f][1][13]-samples[f][1][11]).length for f in window)
            attack_start=next(f for f in window if (samples[f][1][13]-samples[f][1][11]).length>=extension*.96)
            attack_end=min(source_frames-1,attack_start+36)
        elif family=='down_shot':
            # Stop before the standalone get-up recovery, retaining the
            # complete prone firing movement rather than floating it upright.
            flat_height=min(p[1][0].y for p in samples[:source_frames//2])+.1
            attack_end=next((f-1 for f in range(source_frames//2,source_frames)
                if samples[f][1][0].y>flat_height),source_frames-1)
        samples=samples[attack_start:attack_end+1]
        source_rotations=source_rotations[attack_start:attack_end+1]
        source_steps={}
        for i in source_rotations[0]:
            r=np.array([q[i] for q in source_rotations],dtype=np.float64)
            u,_,v=np.linalg.svd(r);r=u@v
            source_steps[str(i)]=np.degrees(np.arccos(np.clip((np.einsum('fij,fij->f',r[1:],r[:-1])-1)/2,-1,1))).tolist()
        frames=len(samples);shot=2
        code=spec['code']
        # Native MOT playback gets complete local channels, so no joint
        # can inherit a translation from the previous native action. The
        # final FBW pass fits the same pose to the captured costume offsets.
        values={(i,k):[] for i in (-1,*ids) for k in range(6)}
        worlds={i:[] for i in ids}
        origins=[]
        previous={}
        for frame,(poses,_) in enumerate(samples):
            origins.append(np.array(poses[0].translation))
            for k in range(6):values[-1,k].append(0.0)
            for i in ids:
                parent=parents[i]
                matrix=poses[i]
                local=poses[parent].inverted()@matrix if parent>=0 else matrix
                euler=euler_xyz(local.to_3x3(),previous.get(i),np);previous[i]=euler
                for k,v in enumerate((*local.translation,*euler)):
                    if (i,k) in values:values[i,k].append(v)
                worlds[i].append(np.array(matrix,dtype='<f4').T.tobytes())
            if frame in (0,shot,frames-1):
                preview[f'{source}:{frame}']={str(i):list(poses[i].translation) for i in mapping}
        header=bytearray((repo/'RetargetHandoff/Clips/Raiden_Reference_MOT/pl0010_2250.mot').read_bytes()[:44])
        header[24:44]=f'pl0010_{code}'.encode().ljust(20,b'\0')
        struct.pack_into('<hII',header,10,frames,44,len(values))
        mot=header+bytearray(12*(len(values)+1))
        for j,((i,k),v) in enumerate(values.items()):
            at=44+12*j;struct.pack_into('<hbbhHI',mot,at,i,k,1,frames,0,len(mot)-at)
            mot.extend(struct.pack(f'<{frames}f',*v))
        struct.pack_into('<h',mot,44+12*len(values),32767)
        gwm=bytearray(header);gwm[:4]=b'FBW1'
        struct.pack_into('<f',gwm,36,bind[0].y)
        struct.pack_into('<II',gwm,12,44,len(ids));struct.pack_into('<f',gwm,20,0)
        gwm.extend(bytes(12*len(ids)))
        for j,i in enumerate(ids):
            at=44+12*j;struct.pack_into('<h2xII',gwm,at,i,frames,len(gwm)-at);gwm.extend(b''.join(worlds[i]))
        error=0
        for frame in range(frames):
            fk=world_pose(skeleton,bytes(mot),frame)
            for i in ids:
                expected=np.frombuffer(worlds[i][frame],dtype='<f4').reshape(4,4).T
                error=max(error,float(np.max(np.abs(fk[i]-expected))))
        if error>.0002:raise ValueError(f'FK verification failed: {error}')
        root=ET.Element('SeqRoot')
        if standing:
            timing_bytes,cues,recovery=timing(frames,shot)
        else:
            recovery=frames-1;cues=[]
            if spec['playable']:
                # Choose separated, outward-facing barrel poses throughout
                # the authored move, rather than fire three early idle frames.
                grip=np.frombuffer(original['wp2062_grip.bin'],dtype='<f4').reshape(4,4)
                def score(f):
                    pose=samples[f][0];wrist=pose[13]
                    barrel=grip@np.array(wrist).T
                    forward=Vector(barrel[2,:3]);outward=wrist.translation-pose[3].translation
                    return forward.dot(outward.normalized())+.5*(1-abs(forward.y))
                for lo,hi in ((.12,.35),(.38,.60),(.65,.86)):
                    choices=range(max(2,round(frames*lo),cues[-1]+8 if cues else 2),min(frames-4,round(frames*hi)))
                    cues.append(max(choices,key=score))
                shot=cues[0]
            timing_bytes=struct.pack('<4sIII'+str(len(cues))+'I',b'PCT2',frames,recovery,len(cues),*cues)
        flags=ET.SubElement(root,'FlagsTrack',SeqNum='2')
        unlock=max((cues[-1]+1) if cues else 1,recovery-2)
        for first,last,flag in ((0,unlock,262144),(unlock,frames-1,2)):
            ET.SubElement(flags,'Seq',LayerFlag='4294967295',StartTime=f'{first/60:.6f}',EndTime=f'{last/60:.6f}',Config='0',Flag0=str(flag),Flag1='0',FreeArg='0 0 0 0')
        target=members if spec['playable'] else auxiliary
        target[f'wp2062_{code}.mot']=bytes(mot)
        target[f'wp2062_{code}.gwm']=bytes(gwm)
        target[f'wp2062_{code}_0_seq.bxm']=encode_bxm(root)
        target[f'wp2062_{code}_timing.bin']=timing_bytes
        print('EXPORTED',code,frames,'frames; FK error',error,'cues',cues,flush=True)
        reports.append(dict(**spec,sha256=hashlib.sha256(fbx).hexdigest(),frames=frames,sourceAttackRange=[attack_start,attack_end],shotFrame=shot,volleyFrames=cues,recoveryFrame=recovery,maxFkMatrixError=error,sourceAngularSteps=source_steps,sourceRestShapeError=rest_error,sourceRestPelvisHeight=solver.restpos[0].y,bindSceneFrame=0))
    packed=pack_members(members)
    assert dict(dat_entries(packed))==members
    assert all(members[n]==b for n,b in original.items() if n not in {f"wp2062_{c['code']}{suffix}" for c in CLIPS for suffix in ('.mot','.gwm','_0_seq.bxm','_timing.bin')})
    if not (output/'wp2062-before-attacks.dat').exists():(output/'wp2062-before-attacks.dat').write_bytes(bank.read_bytes())
    assert len(packed)<32*1024*1024, 'Runtime bank exceeds the bounded native loader capacity'
    assert all(members[n]==b for n,b in original.items() if n.startswith(('wp2062_2f50','wp2062_2f51'))), 'Existing proven attacks changed'
    bank.write_bytes(packed)
    aux_packed=pack_members(auxiliary)
    assert dict(dat_entries(aux_packed))==auxiliary
    (output/'paired-and-contact-review.dat').write_bytes(aux_packed)
    reports.sort(key=lambda c:int(c['code'],16))
    report=dict(referencePath=reference_record['path'],referenceSha256=reference_record['sha256'],packageSha256=package_sha,clips=reports,preview=preview,rootTravelRemoved=True,skeleton=str(skeleton_path.resolve()),skeletonSha256=hashlib.sha256(skeleton).hexdigest(),sourceRig='Humanoid',playableClips=sum(c['playable'] for c in reports),auxiliaryClips=sum(not c['playable'] for c in reports),method='Humanoid rest-basis retarget; full-body FK; axial twist; authored free hands and vertical motion; grounded prone pelvis; support IK only for firing grips',gameplayVerified=False)
    (output/'retarget-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps([dict(code=c['code'],source=c['sourcePath'],frames=c['frames'],maxFkMatrixError=c['maxFkMatrixError'],sourceRestShapeError=c['sourceRestShapeError']) for c in reports],indent=2),flush=True)


if __name__=='__main__':main()
