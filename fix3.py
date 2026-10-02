path = r"E:\GitHub\MGR Raiden with Sam's Murasama Moveset\SamMovesetManager.h"
with open(path, 'r') as f: content = f.read()

content = content.replace('SamUltimatePolicy::Count ', 'SamUltimatePolicy::Count() ')
content = content.replace('SamUltimatePolicy::Count)', 'SamUltimatePolicy::Count())')
content = content.replace('SamUltimatePolicy::Count\n', 'SamUltimatePolicy::Count()\n')

# Manual fixes for the strings
content = content.replace('.name;', '.name.c_str();')
content = content.replace('PlayBossStage(player, SamUltimatePolicy::Moves[m_addonPending].windup)', 'PlayBossStage(player, SamUltimatePolicy::Moves[m_addonPending].windup.c_str())')
content = content.replace('PlayBossStage(player, SamUltimatePolicy::Moves[m_addonPending].release)', 'PlayBossStage(player, SamUltimatePolicy::Moves[m_addonPending].release.c_str())')
content = content.replace('PlayBossStage(player, move.windup ? move.windup : move.release)', 'PlayBossStage(player, !move.windup.empty() ? move.windup.c_str() : move.release.c_str())')
content = content.replace('PlayBossStage(player)', 'PlayBossStage(player, "")')
content = content.replace('PlayBossStage(Pl0000* player, const char* code)', 'PlayBossStage(Pl0000* player, const char* code)\n    {\n        if (!code || !code[0]) return false;')
content = content.replace('if (move.windup)', 'if (!move.windup.empty())')
content = content.replace('const char* windup = SamUltimatePolicy::Moves[m_addonPending].windup;', 'const char* windup = SamUltimatePolicy::Moves[m_addonPending].windup.c_str();')

content = content.replace('SamUltimatePolicy::Moves[m_currentUltimate].name', 'SamUltimatePolicy::Moves[m_currentUltimate].name.c_str()')
content = content.replace('SamUltimatePolicy::Moves[SamUltimatePolicy::UltimateIndex(m_ultimateSelection)].name', 'SamUltimatePolicy::Moves[SamUltimatePolicy::UltimateIndex(m_ultimateSelection)].name.c_str()')

content = content.replace('index % SamUltimatePolicy::UltimateCount', 'SamUltimatePolicy::UltimateCount() ? index % SamUltimatePolicy::UltimateCount() : 0')
content = content.replace('static_cast<int>(SamUltimatePolicy::Count)', 'static_cast<int>(SamUltimatePolicy::Count())')

# Also fix .name.c_str().c_str() in case it was already replaced
content = content.replace('.c_str().c_str()', '.c_str()')
content = content.replace('Count()()', 'Count()')

with open(path, 'w') as f: f.write(content)
