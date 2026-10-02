import re
path = r"E:\GitHub\MGR Raiden with Sam's Murasama Moveset\SamMovesetManager.h"
with open(path, 'r') as f: content = f.read()
content = re.sub(r'PlayBossStage\(player,\s*(.+?)\.windup\)', r'PlayBossStage(player, \1.windup.c_str())', content)
content = re.sub(r'PlayBossStage\(player,\s*(.+?)\.release\)', r'PlayBossStage(player, \1.release.c_str())', content)
content = content.replace('move.windup ? move.windup : move.release', '(!move.windup.empty() ? move.windup.c_str() : move.release.c_str())')
content = content.replace('PlayBossStage(player)', 'PlayBossStage(player, "")') # if any
with open(path, 'w') as f: f.write(content)
