from pathlib import Path
import re
source = Path("dllmain.cpp").read_text()
start = source.index("static int __fastcall Custom_Behavior_RequestAnimationByName")
body = source[start:source.index("const bool nativeSam", start)]
gun = body[body.index("    if (gun) {"):]
assert "ObserveAnimation" in gun
assert "request.clip" not in gun, "Gun must never replace a native movement motion/sequence"
assert "setDirectAnimation" not in gun and "request.owner" not in gun
runtime = Path("SmgSubweapon.h").read_text()
post = runtime[runtime.index("    void PostTick("):]
assert not re.search(r"visualPose\.Apply\([^;]+,false\)", post)
assert "const bool reloading=fire.reload!=0;" in post
assert "const bool dropForReload=reloading && !shootingPoseHeld;" in post
assert "if(!overlay || melee || sliding || traversal || dropForReload)" in post
assert "if(IsEquipped() && gunSummoned && Neutral() && overlay)" in post
assert "if(IsEquipped() && gunSummoned && Neutral() && overlay && !reloadPose && !raisingPose)" in post
assert "const bool combo=Pistol() && PistolAttackActive(player);" in post
assert "AttachAuthoredPistol" in post
print("Gun routing: native movement pairs preserved; no leg overlay; reload/traversal gates retained; pistol attacks mount the authored wrist.")

assert 'pistolAttackActiveCallback' in source and 'PistolAttackActive(player)' in runtime
