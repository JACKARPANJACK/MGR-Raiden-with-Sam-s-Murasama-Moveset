"""Evaluate candidate FBXs to distinguish attacks, paired roles and reactions."""
import json
from pathlib import Path
import bpy

root=Path(__file__).resolve().parents[1]
inventory=json.loads((root/'local_assets/pistol-unity/package-inventory.json').read_text())
rows=[]
for asset in inventory['files']:
    if not (asset['attackCandidate'] and asset['rigType']=='generic' and asset['inplace']):continue
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=asset['local'])
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE' and o.animation_data and o.animation_data.action)
    start,end=rig.animation_data.action.frame_range
    frames=[]
    for fraction in (0,.2,.4,.6,.8,1):
        t=start+(end-start)*fraction;bpy.context.scene.frame_set(int(t),subframe=t-int(t))
        e=rig.evaluated_get(bpy.context.evaluated_depsgraph_get())
        names=['Pelvis','Spine','Spine1','Spine2','Neck','Head','L UpperArm','L Forearm','L Hand','R UpperArm','R Forearm','R Hand','L Thigh','L Calf','L Foot','R Thigh','R Calf','R Foot']
        frames.append({n:list((e.matrix_world@e.pose.bones['Bip001 '+n].matrix).translation) for n in names})
    rows.append(dict(family=asset['family'],range=[start,end],fps=bpy.context.scene.render.fps,poses=frames))
    print(asset['family'],start,end,'pelvisZ',[round(p['Pelvis'][2],2) for p in frames],flush=True)
(root/'local_assets/pistol-unity/source-review.json').write_text(json.dumps(rows,indent=2))
