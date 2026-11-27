"""Rest-basis FBX -> MGR retargeting. Evaluated rotations carry limb twist.

No frame-dependent elbow cross products or reflected rotation matrices are
used. Source rest joint directions calibrate differing bone axes and A/T poses.
"""
from mathutils import Matrix, Vector, Quaternion
from build_stealth_animations_blender import orthobasis, two_bone


class Retarget:
    def __init__(self, rig, mapping, bind, parents, ids, grip_pose, *, standing_shot=True, support_grip=True, weapon_grip=True):
        self.mapping=mapping;self.bind=bind;self.parents=parents;self.ids=ids
        self.standing_shot=standing_shot;self.support_grip=support_grip;self.weapon_grip=weapon_grip
        self.rest={i:rig.matrix_world @ rig.data.bones['Bip001 '+n].matrix_local for i,n in mapping.items()}
        right=self.rest[11].translation-self.rest[7].translation
        right.z=0;right.normalize()
        up=Vector((0,0,1))
        forward=(self.rest[18].translation-self.rest[17].translation)+(self.rest[22].translation-self.rest[21].translation)
        forward.z=0;forward-=right*right.dot(forward);forward.normalize()
        self.axis=Matrix((right,up,forward)).transposed().inverted()
        self.inverse=self.axis.inverted()
        self.restpos={i:self.axis @ m.translation for i,m in self.rest.items()}
        self.align={}
        self.target_rest={}
        for i in ids:
            p=parents[i]
            self.target_rest[i]=self.target_rest[p]+bind[i] if p>=0 else bind[i].copy()
        children={0:1,1:2,2:3,3:4,4:5,6:7,7:8,8:9,10:11,11:12,12:13,
                  15:16,16:17,17:18,19:20,20:21,21:22}
        for hand,base in ((9,256),(13,512)):
            # Metacarpals remain attached to the wrist. FBX finger keys drive
            # the corresponding knuckles rather than the metacarpal helpers.
            children[hand]=base+8
            for first,length in ((base,3),(base+4,3),(base+8,3),(base+12,3),(base+16,3)):
                for j in range(length-1):children[first+j]=first+j+1
        for i in mapping:
            if i in children and children[i] in self.rest:
                child=children[i]
                source=self.restpos[child]-self.restpos[i]
                target=self.target_rest[child]-self.target_rest[i]
                if i in (9,13):
                    base=256 if i==9 else 512
                    srcpole=self.restpos[base+4]-self.restpos[base+16]
                    dstpole=self.target_rest[base+4]-self.target_rest[base+16]
                else:srcpole=dstpole=Vector((0,0,1))
                self.align[i]=orthobasis(source,srcpole) @ orthobasis(target,dstpole).transposed()
            elif 256<=i<=530:
                # Finger roll is defined by the actual parent hand plane.
                parent=parents[i]
                if parent not in mapping:parent=9 if i<512 else 13
                self.align[i]=self.align.get(parent,Matrix.Identity(3))
            else:self.align[i]=Matrix.Identity(3)
        self.scale=(bind[8].length+bind[9].length)/((self.restpos[8]-self.restpos[7]).length+(self.restpos[9]-self.restpos[8]).length)
        self.first=None
        self.grip=grip_pose

    def sample(self, evaluated):
        world={i:evaluated.matrix_world @ evaluated.pose.bones['Bip001 '+n].matrix for i,n in self.mapping.items()}
        pos={i:self.axis @ m.translation for i,m in world.items()}
        rotations={}
        for i,m in world.items():
            current=m.to_quaternion().to_matrix()
            rest=self.rest[i].to_quaternion().to_matrix()
            rotations[i]=self.axis @ current @ rest.transposed() @ self.inverse @ self.align[i]
        if self.first is None:self.first=pos[0].copy()
        # A ground-shot source starts prone. Anchoring that first frame at the
        # standing pelvis height makes the entire prone attack float in midair.
        reference=self.first if self.standing_shot else self.restpos[0]
        pelvis=self.bind[0]+Vector((0,(pos[0].y-reference.y)*self.scale,0))

        def fk():
            out={}
            for i in self.ids:
                p=self.parents[i]
                r=rotations.get(i,out[p].to_3x3() if p>=0 else Matrix.Identity(3))
                fitted_finger=(256<=i<512 and self.support_grip) or (512<=i<=530 and self.weapon_grip)
                if fitted_finger and i in self.grip and p in self.grip:
                    # Keep the hand fitted to the actual pistol handle. The
                    # donor's fingers were fitted to a different weapon mesh.
                    r=out[p].to_3x3()@self.grip[p].to_3x3().transposed()@self.grip[i].to_3x3()
                m=r.to_4x4()
                m.translation=pelvis if i==0 else out[p].translation+out[p].to_3x3()@self.bind[i] if p>=0 else self.bind[i]
                out[i]=m
            return out

        poses=fk()
        if self.support_grip and (pos[9]-pos[13]).length*self.scale<.25:
            relation=self.grip[13].inverted()@self.grip[9]
            rotations[9]=rotations[13]@relation.to_3x3()
            gap=rotations[13]@relation.translation
            goal=poses[13].translation.copy()
            # Project the shared grip into both arms' reachable volume. This
            # avoids separating the support hand when costume proportions differ.
            for _ in range(12):
                for shoulder,offset,a,b in ((11,Vector((0,0,0)),12,13),(7,gap,8,9)):
                    ray=goal+offset-poses[shoulder].translation
                    reach=self.bind[a].length+self.bind[b].length-.0001
                    if ray.length>reach:goal+=ray.normalized()*(reach-ray.length)
            for a,b,c,target in ((11,12,13,goal),(7,8,9,goal+gap)):
                upper,lower,_,_=two_bone(poses[a].translation,target,self.bind[b],self.bind[c],pos[b]-pos[a],Vector((0,0,1)))
                rotations[a]=upper;rotations[b]=lower
        # Skin twist helpers may rotate around the limb's local X axis only.
        # Blending full upper/forearm rotations bakes elbow flexion into the
        # upper-arm skin and produces the collapsing deformation seen in game.
        for a,b,helpers in ((7,8,(1282,1283,1284)),(8,9,(1285,1286,1287)),
                             (11,12,(1289,1290,1291)),(12,13,(1292,1293,1294))):
            relative=(rotations[a].transposed()@rotations[b]).to_quaternion()
            twist=Quaternion((relative.w,relative.x,0,0))
            if twist.magnitude<1e-6:twist=Quaternion()
            twist.normalize()
            for t,i in zip((.15,.5,.85),helpers):
                rotations[i]=rotations[a]@Quaternion().slerp(twist,t).to_matrix()
        poses=fk()
        # Keep the feet on the native standing floor while retaining knee and
        # ankle articulation. Actor translation is authored separately as zero.
        floor=min(self.target_rest[i].y for i in (17,18,21,22))
        offset=floor-min(poses[i].translation.y for i in (17,18,21,22))
        if not self.standing_shot:
            # Correct penetration only: never pull aerial feet back onto the
            # floor, and keep prone head/hips above their contact surfaces.
            offset=max(0,offset,floor+.08-poses[5].translation.y,floor+.09-poses[0].translation.y)
        for i,m in poses.items():
            root=i
            while self.parents[root]>=0:root=self.parents[root]
            if root==0:m.translation.y+=offset
        return poses,pos
