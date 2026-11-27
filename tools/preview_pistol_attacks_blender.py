import bpy,sys,importlib,struct,json
from pathlib import Path
from mathutils import Vector,Matrix
import numpy as np
repo=Path(__file__).resolve().parents[1]
import argparse
p=argparse.ArgumentParser();p.add_argument('--model',required=True);p.add_argument('--label',required=True)
p.add_argument('--codes',nargs='+');p.add_argument('--samples',type=int,default=5);p.add_argument('--resolution',type=int,default=700)
args=p.parse_args(sys.argv[sys.argv.index('--')+1:])
sys.path.insert(0,str(repo/'tools'))
from assets import dat_entries
from gun_motion import world_pose
addon=next(Path(p)/'addons/MGR2Blender2MGR-master' for p in bpy.utils.script_paths() if (Path(p)/'addons/MGR2Blender2MGR-master/__init__.py').exists())
sys.path.insert(0,str(addon.parent));bpy.ops.preferences.addon_enable(module=addon.name)
bpy.types.WindowManager.popup_menu=lambda *a,**kw:print('IMPORT_NOTICE',kw,flush=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.wmb_data(filepath=str(Path(args.model)),reset_blend=False)
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
skeleton=Path(args.model).read_bytes()
before=set(bpy.data.objects)
bpy.ops.import_scene.wmb_data(filepath=str(repo/'Release/PistolModels/wp2060.wmb'),reset_blend=False)
gun_objects=set(bpy.data.objects)-before
gun_roots=[o for o in gun_objects if o.parent not in gun_objects]
gun_basis={o:o.matrix_world.copy() for o in gun_roots}
axis=Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)))

import_mot=importlib.import_module(addon.name+'.mot.importer.motImporter').importMot
members=dict(dat_entries((repo/'Release/GameData/wp/wp2062.dat').read_bytes()))
scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO';scene.display.shading.color_type='SINGLE';scene.display.shading.single_color=(.55,.65,.75)
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.render.resolution_x=args.resolution;scene.render.resolution_y=args.resolution;scene.render.resolution_percentage=100
bpy.ops.object.camera_add(location=(3,-5,2.2));camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,1))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=2.6;scene.camera=camera
for code in (args.codes or ('2f50','2f51')):
 path=repo/f'local_assets/pistol-unity/{code}.mot';path.write_bytes(members['wp2062_'+code+'.mot'])
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
 import_mot(str(path),False)
 frames=struct.unpack_from('<H',members['wp2062_'+code+'.mot'],10)[0]
 for f in sorted(set(np.linspace(0,frames-1,args.samples,dtype=int).tolist())):
  scene.frame_set(f);bpy.context.view_layer.update()
  native=world_pose(skeleton,members['wp2062_'+code+'.mot'],f)
  mapped={int(b.bone['ID']):b for b in rig.pose.bones if 'ID' in b.bone}
  for bone in (0,1,2,3,4,5,7,8,9,11,12,13,15,16,17,19,20,21):
   actual=(rig.matrix_world@mapped[bone].matrix).translation
   expected=axis@Vector((*native[bone][:3,3],1))
   error=(actual-Vector(expected[:3])).length
   if error>.0002:raise ValueError(f'Independent MOT importer mismatch: {code} frame {f} bone {bone}: {error}')
  grip=np.frombuffer(members['wp2062_grip.bin'],dtype='<f4').reshape(4,4)
  barrel=grip@native[13].T
  palm=(native[13][:3,3]+np.mean([native[b][:3,3] for b in (516,520,524,528)],axis=0))*.5
  handling=json.loads((repo/'assets/pistols/handling.json').read_text())
  barrel[3,:3]=palm-np.array(handling['grip_center'])*handling['model_scale']@barrel[:3,:3]
  placed=axis@Matrix(barrel.T.tolist())@axis.inverted()
  for o in gun_roots:o.matrix_world=placed@gun_basis[o]
  scene.render.filepath=str(repo/f'local_assets/pistol-unity/review-{args.label}-{code}-{f:02}.png')
  bpy.ops.render.render(write_still=True)
print('RENDER_COMPLETE',flush=True)
