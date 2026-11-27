"""Compare exported rotation continuity directly with the Humanoid FBXs.

Use normalized quaternions in double precision. Trace/acos on float32 matrix
products invents small rotation steps near zero and is unsuitable for this test.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys
import bpy
import numpy as np
from mathutils import Matrix

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from assets import dat_entries

NAMES={0:'Pelvis',1:'Spine',2:'Spine1',3:'Spine2',4:'Neck',5:'Head',
       6:'L Clavicle',7:'L UpperArm',8:'L Forearm',9:'L Hand',
       10:'R Clavicle',11:'R UpperArm',12:'R Forearm',13:'R Hand',
       15:'L Thigh',16:'L Calf',17:'L Foot',18:'L Toe0',
       19:'R Thigh',20:'R Calf',21:'R Foot',22:'R Toe0'}


def steps(quaternions):
    q=np.asarray(quaternions,dtype=np.float64)
    q/=np.linalg.norm(q,axis=1)[:,None]
    return np.degrees(2*np.arccos(np.clip(np.abs(np.einsum('fi,fi->f',q[1:],q[:-1])),0,1)))


def main():
    out=ROOT/'local_assets/pistol-unity';report_path=out/'retarget-report.json'
    report=json.loads(report_path.read_text())
    inventory=json.loads((out/'package-inventory.json').read_text())
    records={r['path']:r for r in inventory['files']}
    banks=[dict(dat_entries((ROOT/'Release/GameData/wp/wp2062.dat').read_bytes())),
           dict(dat_entries((out/'paired-and-contact-review.dat').read_bytes()))]
    tested=0;worst=0
    # Reverse order independently of the exporter to catch timeline contamination.
    for clip in reversed(report['clips']):
        record=records[clip['sourcePath']]
        assert record['rigType']=='humanoid' and '/Humanoid/' in record['path']
        assert hashlib.sha256(Path(record['local']).read_bytes()).hexdigest()==clip['sha256']
        bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
        bpy.context.scene.frame_set(0)
        bpy.ops.import_scene.fbx(filepath=record['local'],use_image_search=False)
        for obj in list(bpy.context.scene.objects):
            if obj.type in ('MESH','CAMERA','LIGHT'):bpy.data.objects.remove(obj,do_unlink=True)
        rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE' and o.animation_data and o.animation_data.action)
        start,_=rig.animation_data.action.frame_range
        fps=bpy.context.scene.render.fps/bpy.context.scene.render.fps_base
        first,last=clip['sourceAttackRange'];source={i:[] for i in NAMES}
        for f in range(first,last+1):
            t=start+f*fps/60;bpy.context.scene.frame_set(int(t),subframe=t-int(t))
            evaluated=rig.evaluated_get(bpy.context.evaluated_depsgraph_get())
            for i,name in NAMES.items():
                matrix=evaluated.matrix_world@evaluated.pose.bones['Bip001 '+name].matrix
                source[i].append(tuple(matrix.to_quaternion()))
        gwm=banks[0 if clip['playable'] else 1]['wp2062_'+clip['code']+'.gwm']
        native={}
        for j in range(struct.unpack_from('<I',gwm,16)[0]):
            at=44+j*12;i,n,offset=struct.unpack_from('<h2xII',gwm,at)
            if i not in NAMES:continue
            rows=np.frombuffer(gwm,dtype='<f4',count=16*n,offset=at+offset).reshape(n,4,4)
            native[i]=[tuple(Matrix(m[:3,:3].T.tolist()).to_quaternion()) for m in rows]
        source_steps={str(i):steps(q).tolist() for i,q in source.items()}
        error=0
        for i in NAMES:
            if clip['family'].startswith('pistol_attack') and i in (7,8,9,11,12):continue
            difference=np.abs(steps(native[i])-np.asarray(source_steps[str(i)]))
            error=max(error,float(difference.max()))
        assert error<.01,(clip['code'],'Retarget introduced an angular jump',error)
        clip['sourceAngularSteps']=source_steps;clip['maxSourceStepErrorDegrees']=error
        tested+=clip['frames'];worst=max(worst,error)
        print('SOURCE_MATCH',clip['code'],clip['frames'],'frames; max step error',error,flush=True)
    report['sourceContinuityVerified']=True
    report['sourceContinuityFrames']=tested
    report['maxSourceStepErrorDegrees']=worst
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS:',tested,'exported frames independently checked against Humanoid FBXs; max angular-step error',worst,flush=True)


if __name__=='__main__':main()
