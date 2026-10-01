#pragma once
#include "1-GameFunctionManager.h"
#include "1-EffectManager.h"
#include "1-GlobleVarious.h"

class BossEnemyManager
{
private:
	static constexpr float POSITION_THRESHOLD = 0.09f;
	static constexpr float RAGE_DURATION = 1800.0f;
	static constexpr float ARMSTRONG_CLOSE_RANGE = 5.0f;

	inline static bool b_BossSamSwordLighting = false;
	inline static bool b_RepeatMistralPos = false;

	static inline const std::unordered_set<unsigned int> EXCLUDED_MODELS = {
		0x20033, 0x20200, 0x20060
	};

	static inline const std::unordered_map<unsigned int, unsigned int> ACTION_RESTRICTIONS = {
		{0x10000F, 0x100103}, {0x100011, 0x100104}, {0x100010, 0x100103},
		{0x100015, 0x100104}, {0x100014, 0x100104}, {0x100012, 0x100109},
		{0x100013, 0x100110}
	};

	static inline const std::unordered_set<unsigned int> SPECIAL_PHASES = {
		0x740, 0x470, 0xA15, 0x138, 0xA50
	};

	static inline const std::vector<std::pair<int, unsigned int>> BOSS_SAM_ACTIONS = {
		{3, 0x30003}, {6, 0x30004}, {9, 0x30005}, {12, 0x30006},
		{15, 0x30025}, {18, 0x30024}, {21, 0x30001}, {24, 0x30002},
		{33, 0x3000A}, {42, 0x3000F}, {45, 0x30010},
		{48, 0x30012}, {51, 0x30014}, {57, 0x30016}, {60, 0x3001A}
	};

	static inline thread_local std::mt19937 m_gen{ std::random_device{}() };
	static inline thread_local std::uniform_int_distribution<int> m_actionDist{ 1, 60 };

public:
	[[nodiscard]] static BossEnemyManager& GetInstance() noexcept
	{
		static BossEnemyManager instance;
		return instance;
	}

	void Reset_EnemyHandles_StaticVariable()
	{
		b_BossSamSwordLighting = false;
		b_RepeatMistralPos = false;
	}

	void CheckExecuteForEnemy() {}

	void Em0310_HandleActions(BehaviorEmBase* enemy)
	{
		if (!enemy) return;
		EnemyQTEActionHandles(enemy);
	}

	void Em0020_HandleActions(BehaviorEmBase* enemy)
	{
		if (!enemy) return;
		EnemyQTEActionHandles(enemy);
		HandleModel_0x20020(enemy);
	}

	void Em0110_HandleActions(BehaviorEmBase* enemy)
	{
		if (!enemy) return;
		HandleModel_0x20110(enemy);
	}

	void Em0700_HandleActions(BehaviorEmBase* enemy)
	{
		if (!enemy) return;
		HandleModel_0x20700(enemy);
	}

	void CheckRage(Pl0000* player) {}

private:
	void EnemyQTEActionHandles(BehaviorEmBase* enemy)
	{
		if (!enemy) return;

		Pl0000* player = g_GameUIManager.m_pPlayer;
		if (!player) return;

		static const std::unordered_map<unsigned int, std::vector<std::pair<int, int>>> QTE_ANIM_MAP = {
			{0x100500, {{10, 1339}, {12, 1369}}},
			{0x100501, {{10, 1340}, {12, 1368}}},
			{0x100502, {{10, 1338}, {12, 1370}, {14, 1368}}},
			{0x100504, {{10, 1372}, {12, 1368}}}
		};

		const auto it = QTE_ANIM_MAP.find(enemy->m_Rno0);
		if (it != QTE_ANIM_MAP.end())
		{
			for (const auto& [actionId, animId] : it->second)
			{
				if (enemy->m_Rno1 == actionId)
				{
					enemy->requestAnimationByMap(animId, player->m_pEntity, 0, 0.0f, 1.0f, 0x8038000, -1.0f, 1.0f);
					++enemy->m_Rno1;
					return;
				}
			}
		}

		static const std::unordered_set<int> VALID_IDS = { 11, 13, 14, 15 };
		if (VALID_IDS.count(enemy->m_Rno1))
		{
			g_GameFunctionManager.sub_6C80A0(enemy, 1.0f, 1.0f);
			if (g_GameFunctionManager.IsAnimationEnded(enemy, 0))
			{
				if (enemy->m_Rno1 == 14 || enemy->m_Rno1 == 15)
				{
					static const std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>> FINAL_STATES = {
						{0x20020, {0x20003, 0x20020}},
						{0x20310, {0x10001, 0x20310}},
						{0x20110, {0x10002, 0x20110}}
					};
					const auto stateIt = FINAL_STATES.find(enemy->m_ModelIndex);
					if (stateIt != FINAL_STATES.end())
					{
						enemy->setRno(stateIt->second.first, 0, 0, 0);
						enemy->m_ObjId = static_cast<eObjID>(stateIt->second.second);
					}
				}
				else
					++enemy->m_Rno1;
			}
		}
	}

	void HandleModel_0x20020(BehaviorEmBase* enemy)
	{
		if (!enemy) return;

		Pl0000* player = enemy->m_pEnemy;
		if (!player) return;

		Em0020* bossSam = static_cast<Em0020*>(enemy);
		if (!bossSam->m_pEntity) return;

		bossSam->m_pEntity->getSlowRate()->m_pUnit->m_Rate = Trigger::StaFlags.STA_QTE ? 1.0f : 0.893333f;

		const float distance = (GameFunctionManager::GetInstance().GetPositionFromMatrix(bossSam->m_pRootParts) -
			GameFunctionManager::GetInstance().GetPositionFromMatrix(player->m_pRootParts)).length();

		bool isIdleState = (bossSam->m_Rno0 == 0x20000) ||
			(bossSam->m_pAnimationSlot && bossSam->m_pAnimationSlot->m_pArray &&
				strcmp(bossSam->m_pAnimationSlot->m_pArray[0].m_pAnimName, "0000") == 0);

		if (isIdleState && !player->isInAir() && distance <= 3.0f)
		{
			const int randomAction = m_actionDist(m_gen);
			for (const auto& [threshold, action] : BOSS_SAM_ACTIONS)
				if (randomAction <= threshold)
				{
					if (isIdleState)
						g_GameFunctionManager.sub_20B80(bossSam, action, 0, 0, 0, 0);
					break;
				}
		}

		if (!b_BossSamSwordLighting)
		{
			bossSam->createEffect(600, &g_EffectHandle->m_BossSamSwordEffectController);
			b_BossSamSwordLighting = true;
		}

		if (player->m_Rno0 == 0xF5 && bossSam->m_Rno1 == 7)
			if (bossSam->m_AnimationFrame <= 1.0f)
			{
				bossSam->createEffect(601, &g_EffectHandle->m_BossSamRipperEffectController);
				bossSam->createEffect(602, &g_EffectHandle->m_BossSamRipperEffectController);
				g_GameFunctionManager.Play_Sound("core_se_btl_ripper_in", bossSam, -1, 0);
			}

		if (bossSam->m_Rno0 == 0x200008 &&
			bossSam->m_Rno1 == 10 &&
			bossSam->m_AnimationFrame >= 165 &&
			bossSam->m_AnimationFrame <= 166)
		{
			g_EffectHandle->m_BossSamRipperEffectController.FadeUnits(30.0f, 0.0f);
			g_GameFunctionManager.CreateEffect(bossSam, 102, &g_EffectHandle->m_BossSamRipperEffectController);
			g_GameFunctionManager.Play_Sound("core_se_btl_ripper_out", bossSam, -1, 0);
		}

		if (bossSam->field_1370)
			g_EffectHandle->m_BossSamSwordEffectController.FadeUnits(5.0f, 0.0f);

		Entity* bossRaidenSword = bossSam->m_BladeHandle.getEntity();
		cParts* bone = bossSam->getPartsPtrNo(0x720u);

		if (!bone || !bossRaidenSword || (bossSam->field_1370 && player->m_Rno0 == 0xF5))
			return;

		const int baseBoneID = 32;
		bossSam->setConstraintsBone(4, baseBoneID, 0);

		const float posX = bone->m_TransPos.x;
		const float posY = bone->m_TransPos.y;
		int finalBoneID = baseBoneID;

		bool animValid = bossSam->m_pAnimationSlot && bossSam->m_pAnimationSlot->m_pArray;
		const char* animName = animValid ? bossSam->m_pAnimationSlot->m_pArray[0].m_pAnimName : "";

		if (posX < -POSITION_THRESHOLD ||
			(animValid && (strcmp(animName, "3016") == 0 || strcmp(animName, "92b0") == 0)) ||
			bossSam->m_Rno0 < 0x30000 ||
			bossSam->m_Rno0 > 0x40000)
			finalBoneID = baseBoneID;
		else if (posX > POSITION_THRESHOLD)
			finalBoneID = 1793;
		else if (posY > POSITION_THRESHOLD ||
			(animValid && strcmp(animName, "2065") == 0 && bossSam->m_AnimationFrame <= 280.0f))
			finalBoneID = 1794;
		else if (posY < -POSITION_THRESHOLD)
			finalBoneID = 1795;

		bossSam->removeConstraint(4);
		bossSam->attachObject(4, bossSam->m_pEntity, bossRaidenSword, finalBoneID, 0);
	}

	void HandleModel_0x20110(BehaviorEmBase* enemy)
	{
		if (!enemy) return;

		Pl0000* player = enemy->m_pEnemy;
		if (!player) return;

		const int animationMapID = player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray ?
			player->m_pAnimationSlot->m_pArray[0].field_4 : 0;

		Behavior* playerSheath = player->m_pSheathEntity ? player->m_pSheathEntity->m_pBehavior : nullptr;

		if (player->m_Rno0 == 0x105 && player->m_Rno1 == 5)
		{
			if (!b_RepeatMistralPos && playerSheath)
			{
				playerSheath->requestAnimationByName("9e98", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				b_RepeatMistralPos = true;
			}
		}
		else
			b_RepeatMistralPos = false;

		if (animationMapID == 271 && player->m_Rno1 == 6)
		{
			const int frame = player->m_AnimationFrame;

			if (frame >= 485.0f)
			{
				player->removeConstraint(4);

				if (player->m_SwordHandle.getEntity())
					player->attachObject(4, player->m_pSheathEntity, player->m_SwordHandle.getEntity(), 1808, 0);

				player->field_A00.FadeUnits(5.0f, 0.0f);
			}

			if (frame >= 375.0f && frame <= 377.0f)
				g_GameFunctionManager.Play_Sound("pl1400_se_swd_swish_blood", player, -1, 0);

			if (frame >= 460.0f && frame <= 462.0f)
				g_GameFunctionManager.Play_Sound("pl1400_se_swd_scabbard_sheathe_s", player, -1, 0);

			player->m_pEntity->getSlowRate()->m_pUnit->m_Rate = (frame >= 730.0f) ? 0.0f : 1.0f;

			if (frame >= 730.0f)
				g_GameFunctionManager.NextMission(&PhaseManager::ms_Instance, "P170_END", 1, 0);
		}
	}

	void HandleModel_0x20700(BehaviorEmBase* enemy)
	{
		if (!enemy) return;

		Pl0000* player = enemy->m_pEnemy;
		if (!player) return;

		Em0700* armstrong = static_cast<Em0700*>(enemy);
		if (!armstrong) return;

		if (player->isUnarmed())
		{
			const auto it = ACTION_RESTRICTIONS.find(player->m_Rno0);
			if (it != ACTION_RESTRICTIONS.end())
				player->setRno(it->second, 0, 0, 0);
		}

		if (armstrong->m_Rno0 == 0x30003)
			armstrong->setRno(0x20008, 0, 0, 0);
	}

	BossEnemyManager() = default;
	~BossEnemyManager() = default;
	BossEnemyManager(const BossEnemyManager&) = delete;
	BossEnemyManager& operator=(const BossEnemyManager&) = delete;
};

inline BossEnemyManager* g_BossEnemyManager = &BossEnemyManager::GetInstance();