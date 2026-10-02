import re
path = r"E:\GitHub\MGR Raiden with Sam's Murasama Moveset\SamMovesetManager.h"
with open(path, 'r') as f: content = f.read()

content = re.sub(r'Moves\[(.*?)\].name(?!.c_str)', r'Moves[\1].name.c_str()', content)
content = content.replace('PlayBossStage(player, "")', 'PlayBossStage(player, "")') # wait
content = re.sub(r'PlayBossStage\(player\)', r'PlayBossStage(player, "")', content)
content = re.sub(r'PlayBossStage\(player, \(!move.windup.empty\(\) \? move.windup.c_str\(\) : move.release.c_str\(\)\)', r'PlayBossStage(player, (!move.windup.empty() ? move.windup.c_str() : move.release.c_str()))', content) # no wait, ternary inside function call
content = content.replace('PlayBossStage(player, !move.windup.empty() ? move.windup.c_str() : move.release.c_str())', 'PlayBossStage(player, !move.windup.empty() ? move.windup.c_str() : move.release.c_str())')

# Let's fix line 1163
content = content.replace('if (move.windup)', 'if (!move.windup.empty())')
content = content.replace('const char* windup = SamUltimatePolicy::Moves[m_addonPending].windup;', 'const char* windup = SamUltimatePolicy::Moves[m_addonPending].windup.c_str();')

with open(path, 'w') as f: f.write(content)
