#pragma once
#include "1-GameFunctionManager.h"
#include "1-ChangePlayerManager.h"
#include "1-GlobleVarious.h"
#include "1-PlayerActionHandler.h"
#include <unordered_set>

class PlayerStateManager
{
public:
	static PlayerStateManager& GetInstance()
	{
		static PlayerStateManager instance;
		return instance;
	}

	void Reset_PlayerActionHandles_StaticVariable()
	{
		if (!g_pPlayerActionHandler)
			return;

		g_pPlayerActionHandler->b_30025_1 = 0;
		g_pPlayerActionHandler->b_30025_2 = 0;
		g_pPlayerActionHandler->b_30025_3 = 0;
		g_pPlayerActionHandler->b_resetSheathAction = 0;
		g_pPlayerActionHandler->b_resetPos_arm_1 = 0;
		g_pPlayerActionHandler->b_MistralFlag = 0;
	}

	void UpdateQTEState(Pl0000* player)
	{
		if (!player)
			return;

		static auto& gameStateManager = GameStateManager::GetInstance();
		static auto& gameFunctionManager = GameFunctionManager::GetInstance();

		if (player->isOnGround())
			player->field_314 = 0;

		injector::WriteMemory<unsigned int>(
			shared::base + 0x129EC28,
			(player->m_Rno0 == 69) ? shared::base + 0x7DA310 : shared::base + 0x46B890,
			true
		);

		if (gameFunctionManager.IsNinjaRunEvent())
			return;

		uint32_t currentAction = player->m_Rno0;

		if (player->m_OldRno0 == 0x46 && player->m_Rno0 == 0x100000)
		{
			((void(__thiscall*)(Pl1400*))(shared::base + 0x492EB0))((Pl1400*)player);
			player->setRno(0x0, 0, 0, 0);
		}

		static const std::unordered_set<uint32_t> idleActions = {
			3, 4, 12, 79, 81, 91, 93, 97, 102, 105, 110, 111, 116, 117
		};

		if (currentAction == 0 && player->m_Rno1 != 1 && !player->field_3458 && !Trigger::StaFlags.STA_QTE)
			player->setIdle(0);
		else if (idleActions.count(currentAction))
			player->setIdle(0);
	}

	void PlayerSheathSwordSpeedFix(Pl0000* player)
	{
		if (!player || !player->m_pAnimationSlot || !player->m_pSheathEntity || !player->m_SwordHandle.getEntity())
			return;

		float rate = 1.0f;
		if (player->m_pAnimationSlot->m_pArray[0].m_pAnimName)
		{
			const char* animName = player->m_pAnimationSlot->m_pArray[0].m_pAnimName;
			if (strstr(animName, "9100") || strstr(animName, "9101") || strstr(animName, "9102") ||
				strstr(animName, "9030") || strstr(animName, "9032") || strstr(animName, "9038") ||
				strstr(animName, "9040") || strstr(animName, "9042") || strstr(animName, "9048") ||
				strstr(animName, "9002") || strstr(animName, "9003") || strstr(animName, "9108"))
				rate = 3.0f;
		}

		if (auto* slowRateUnit = player->m_pSheathEntity->getSlowRate())
			slowRateUnit->m_pUnit->m_Rate = rate;

		if (auto* swordSlowRateUnit = player->m_pSheathEntity->getSlowRate())
			swordSlowRateUnit->m_pUnit->m_Rate = rate;
	}

	void HandleThreeSlashCombo(Pl0000* player)
	{
		if (!player)
			return;

		static auto& gameStateManager = GameStateManager::GetInstance();

		if (!gameStateManager.IsStyleChanged || player->field_940)
			return;

		if (!player->isInAir() && player->m_Rno0 == 0x100005)
			player->setIdle(0);

		if (player->m_Rno0 != 0x100010 && player->m_Rno1 == 1 &&
			!g_pPlayerActionHandler->b_30025_1 && !g_pPlayerActionHandler->b_30025_3)
			g_pPlayerActionHandler->b_30025_1 = true;

		if (player->m_Rno0 == 0x100010 && player->m_Rno1 == 3 &&
			g_pPlayerActionHandler->b_30025_1 && !g_pPlayerActionHandler->b_30025_2 && !g_pPlayerActionHandler->b_30025_3)
		{
			player->setRno(0x100010, 1, 0, 0);
			g_pPlayerActionHandler->b_30025_2 = true;
		}

		if (player->m_Rno0 == 0x100010 && player->m_Rno1 == 9 &&
			g_pPlayerActionHandler->b_30025_1 && g_pPlayerActionHandler->b_30025_2 && !g_pPlayerActionHandler->b_30025_3)
		{
			g_pPlayerActionHandler->b_30025_2 = true;
			g_pPlayerActionHandler->b_30025_3 = true;
		}

		if (player->m_Rno0 != 0x100010)
		{
			g_pPlayerActionHandler->b_30025_1 = false;
			g_pPlayerActionHandler->b_30025_2 = false;
			g_pPlayerActionHandler->b_30025_3 = false;
		}

		g_pPlayerActionHandler->b_30025_1 = true;
	}

	void HandleRipperMode(Pl0000* player)
	{
		if (!player)
			return;

		static auto& gameFunctionManager = GameFunctionManager::GetInstance();

		unsigned int currentPhase = PhaseManager::ms_Instance.getCurrentSubPhase();
		if (currentPhase == 0x380)
			return;

		auto* gameFlagsPtr = reinterpret_cast<DWORD*>(&Trigger::GameFlags);

		if (player->m_bRipperModeEnabled == 0)
		{
			if ((*reinterpret_cast<WORD*>(gameFlagsPtr + 2) & 0x800) != 0)
				player->enableRipperMode();
			else if (player->field_31A0 < 0.0 && (*gameFlagsPtr & 0x40000000) == 0)
			{
				bool canActive = gameFunctionManager.CanActiveRipper(player);
				bool inputCondition1 = (static_cast<int>(player->m_fInputMagnitudeSquared) & 0x1000) != 0 &&
					(player->m_CurrentInput.m_nButtonsPressed & 0x8000) != 0;
				bool inputCondition2 = (player->m_CurrentInput.m_nButtonsPressed & 0x1000) != 0 &&
					(static_cast<int>(player->m_fInputMagnitudeSquared) & 0x8000) != 0;
				bool buttonCondition = ((int(__cdecl*)(Hw::eSaveKeybind))(shared::base + 0x61D2D0))(Hw::KEYBIND_RIPPERMODE);

				if (canActive && (inputCondition1 || inputCondition2 || buttonCondition))
					player->enableRipperMode();
			}
		}
		else if (player->m_bRipperModeEnabled == 1)
		{
			if ((*reinterpret_cast<WORD*>(gameFlagsPtr + 2) & 0x800) == 0)
			{
				float fuelConsumption = player->field_319C * player->m_SpeedRate;
				gameFunctionManager.FuelGaugeGone(player, fuelConsumption);

				if (player->getFuelContainer() == 0.0)
					player->disableRipperMode(1);

				if (player->field_31A4 < 0.0)
				{
					int inputState = static_cast<int>(player->m_fInputMagnitudeSquared);
					if (((inputState & 0x1000) != 0 && (player->m_CurrentInput.m_nButtonsPressed & 0x8000) != 0) ||
						((player->m_CurrentInput.m_nButtonsPressed & 0x1000) != 0 && (inputState & 0x8000) != 0) ||
						((int(__cdecl*)(Hw::eSaveKeybind))(shared::base + 0x61D2D0))(Hw::KEYBIND_RIPPERMODE))
						player->disableRipperMode(1);
				}
			}
		}
	}

	void UpdateSheathState(Pl0000* player)
	{
		if (!player)
			return;

		static auto& gameStateManager = GameStateManager::GetInstance();
		static auto& gameFunctionManager = GameFunctionManager::GetInstance();

		if (Trigger::StaFlags.STA_QTE || player->isUnarmed())
			return;

		unsigned int currentAction = player->m_Rno0;
		if (player->isCodecTalk() || currentAction == 0x10002E || currentAction == 0x10000E || currentAction == 0x100008)
			return;

		if (!player->m_pSheathEntity)
			return;

		Behavior* PlayerSheath = static_cast<Behavior*>(player->m_pSheathEntity->m_pBehavior);
		if (!PlayerSheath)
			return;

		if (gameStateManager.IsStyleChanged && !g_pPlayerActionHandler->b_resetSheathAction)
		{
			PlayerSheath->requestAnimationByMap(player->m_bSwordHidden ? 4 : 5);
			g_pPlayerActionHandler->b_resetSheathAction = true;
		}
		else if (!gameStateManager.IsStyleChanged)
			g_pPlayerActionHandler->b_resetSheathAction = false;
	}

	void UpdateDamageVisuals(Pl0000* player)
	{
		if (!player)
			return;

		bool showDamage = (player->m_Hp < player->m_HpMax * 1.5f);

		player->toggleAnyMesh("_dam1_CBODY", showDamage);
		player->toggleAnyMesh("_dam1_RBODY", showDamage);
		player->toggleAnyMesh("_dam1_LBODY", showDamage);
	}

	void HandleAdditionPlayerActions(Pl0000* player)
	{
		if (!player)
			return;

		if (player->isCodecTalk() || player->isBladeModeActive() || Trigger::StaFlags.STA_QTE)
			return;

		unsigned int currentAction = player->m_Rno0;
		if (currentAction == 0x10002B || currentAction == 0x100007 || currentAction == 0x100064 ||
			currentAction == 0x10000E || currentAction == 0x10001D || currentAction == 0x100018 ||
			currentAction > 0x100090)
			return;

		if (!player->isInAir())
			HandleGroundAttacks(player);

		if (shared::IsKeyPressed('H', 1))
			player->setRno(player->isInAir() ? 0x100018 : 0x100096, 0, 0, 0);

		if (shared::IsKeyPressed('Y', 1) && !player->isInAir())
			player->setRno(0x100064, 0, 0, 0);
	}

	// HandlePlayerFlags operates on Pl1400's private fields after 0x5400.
	// Since we use a Pl0000 allocation, keep this safe by only calling it
	// when we can guarantee the memory layout is correct.
	void HandlePlayerFlags(Pl0000*) {}

	void ResetComboState(Pl0000* player)
	{
		if (!player)
			return;

		player->field_25F8 = 0;
		player->field_25F4 = 0;
		player->field_2568 = 0;
		player->field_25E8 = 0;
		if (player->field_2620 >= 5)
			player->field_2620 = 0;
	}

	void HandleGroundAttacks(Pl0000* player)
	{
		if (!player)
			return;

		static auto& gameStateManager = GameStateManager::GetInstance();
		static auto& gameFunctionManager = GameFunctionManager::GetInstance();

		if (player->field_25F4 && (player->m_ButtonLightAttack & player->m_CurrentInput.m_nButtonsPressed))
		{
			ResetComboState(player);
			if (gameStateManager.IsQTEEnabled && !player->isInAir())
			{
				int state = gameStateManager.IsStyleChanged ? 0x100101 : 0x100100;
				player->setRno(state, 0, 0, 0);
			}
			else
				player->setRno(0x100093, 0, 0, 0);
		}

		if ((player->m_ButtonLightAttack & player->m_CurrentInput.m_nButtonsPressed) &&
			(player->m_ButtonHeavyAttack & player->m_CurrentInput.m_nButtonsPressed))
		{
			ResetComboState(player);
			gameStateManager.IsSamuraiWarningActive = true;

			if (!gameStateManager.IsQTEEnabled && player->field_1370.m_TargetHandle.getEntity())
				player->setRno(0x100107, 0, 0, 0);

			if (gameStateManager.IsQTEEnabled)
				player->setRno(0x100090, 0, 0, 0);
		}
	}

private:
	PlayerStateManager() = default;
	~PlayerStateManager() = default;

	PlayerStateManager(const PlayerStateManager&) = delete;
	PlayerStateManager& operator=(const PlayerStateManager&) = delete;
	PlayerStateManager(PlayerStateManager&&) = delete;
	PlayerStateManager& operator=(PlayerStateManager&&) = delete;

};

inline PlayerStateManager* g_pPlayerStateManager = &PlayerStateManager::GetInstance();
