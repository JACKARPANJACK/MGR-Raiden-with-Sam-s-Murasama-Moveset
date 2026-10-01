#pragma once
#include "1-GameFunctionManager.h"
#include "1-ChangePlayerManager.h"
#include <Em0080.h>
#include <Em0200.h>
#include <Em0600.h>

class EventTriggerHandle
{
private:
	EventTriggerHandle() = default;
	~EventTriggerHandle() = default;

	inline static bool b_IsSwordInSheathed = false;
	inline static bool b_isMonsoonPhaseRipperMode = false;
	inline static bool ResetFlagOnce = false;

	template<typename T>
	static bool IsInRange(T value, T min, T max) noexcept
	{
		return value >= min && value <= max;
	}

	static bool IsSpecialPhase(uint32_t currentPhase) noexcept
	{
		static constexpr uint32_t SPECIAL_PHASES[] = { 0x740, 0x470, 0xA15, 0x138, 0xA50 };
		for (uint32_t phase : SPECIAL_PHASES)
			if (currentPhase == phase)
				return true;
		return false;
	}

	static void ChangeMission(std::string_view phase, uint32_t subPhase) noexcept
	{
		static constexpr DWORD BASE_PHASE_ID = 0x1764670;
		static constexpr DWORD BASE_PHASE_NAME = 0x1764674;
		static constexpr DWORD DLC_PHASE_ID = 0x1766004;
		static constexpr DWORD DLC_PHASE_NAME = 0x1766008;
		static constexpr size_t PHASE_NAME_SIZE = 30;

		if (!shared::base) return;

		bool& isDLCPhaseActive = g_GameStateManager.IsDLCPhaseActive;
		isDLCPhaseActive = (subPhase & 0xF00) == 0xC00 || (subPhase & 0xF00) == 0xD00;

		if (!isDLCPhaseActive)
		{
			*(uint32_t*)(shared::base + DLC_PHASE_ID) = 0;
			*((char*)(shared::base + DLC_PHASE_NAME)) = '\0';
		}

		DWORD phaseIdAddress = isDLCPhaseActive ? DLC_PHASE_ID : BASE_PHASE_ID;
		DWORD phaseNameAddress = isDLCPhaseActive ? DLC_PHASE_NAME : BASE_PHASE_NAME;

		*(uint32_t*)(shared::base + phaseIdAddress) = subPhase;

		char* dest = (char*)(shared::base + phaseNameAddress);
		size_t toCopy = phase.size() < (PHASE_NAME_SIZE - 1) ? phase.size() : (PHASE_NAME_SIZE - 1);
		if (toCopy > 0)
			std::copy_n(phase.data(), toCopy, dest);
		dest[toCopy] = '\0';
	}

	static uint32_t GetBossAction() noexcept
	{
		if (!shared::base) return 0;
		uint32_t* bossActionPtr = (uint32_t*)(shared::base + 0x014A5024);
		if (!bossActionPtr || !*bossActionPtr) return 0;
		bossActionPtr = (uint32_t*)(*bossActionPtr);
		if (!bossActionPtr) return 0;
		return *(uint32_t*)(*bossActionPtr + 0x618);
	}

public:
	EventTriggerHandle(const EventTriggerHandle&) = delete;
	EventTriggerHandle& operator=(const EventTriggerHandle&) = delete;

	static EventTriggerHandle& GetInstance() noexcept
	{
		static EventTriggerHandle instance;
		return instance;
	}

	void Reset_EventTriggerForSam_StaticVariable()
	{
		b_IsSwordInSheathed = false;
		b_isMonsoonPhaseRipperMode = false;
		ResetFlagOnce = false;
	}

	void ProcessGameFixes(Pl0000* player) noexcept
	{
		if (!player || !g_GameStateManager.IsMainSamPlayer) return;

		uint32_t currentPhase = PhaseManager::ms_Instance.getCurrentSubPhase();
		uint32_t& currentAction = player->m_Rno0;

		if (currentAction == 0x134 && IsInRange(currentPhase, 0x300u, 0x400u))
			player->setRno(0x10000E, 0, 0, 0);

		if (currentPhase == 0x370 && player->m_pBladeEntity && player->m_pBladeEntity->m_pBehavior)
			((Behavior*)player->m_pBladeEntity->m_pBehavior)->offDisp();

		if (g_GameFunctionManager.CheckPhase("P138_BREAKDOWN_1") ||
			g_GameFunctionManager.CheckPhase("P138_BREAKDOWN_2") ||
			g_GameFunctionManager.CheckPhase("P138_RE_START"))
			ChangeMission("P138_HELI01_START", 0x138);

		if (g_GameFunctionManager.CheckPhase("btl_sam_3"))
		{
			if ((currentAction != 0x100051 && currentAction != 0x10004E) && currentAction >= 0x100000)
				player->setRno(0, 0, 0, 0);
			else if (currentAction == 0x100051 || currentAction == 0x10004E)
				player->setRno(0x201, 0, 0, 0);
		}

		if (currentAction != 0x100008 && player->field_13F4 && !b_IsSwordInSheathed)
		{
			float frame = (float)player->m_AnimationFrame;
			if (frame > 1.0f && frame < 2.0f)
				g_GameFunctionManager.Play_Sound("pl1400_se_swd_scabbard_sheathe_m", player, -1, 0);
			b_IsSwordInSheathed = true;
		}

		if (currentAction != 0x100008)
			b_IsSwordInSheathed = false;

		if (currentPhase == 0x380 && !b_isMonsoonPhaseRipperMode)
		{
			player->enableRipperMode();
			player->setFuelLevel(player->getFuelCapacity(0));
			b_isMonsoonPhaseRipperMode = true;
		}

		Trigger::GameFlags.GAME_DEPRESSSION_RAIDEN = false;
	}

	void RestoreRaidenRoutes() noexcept
	{
		if (!shared::base)
			return;

		GameFunctionManager& funcMgr = g_GameFunctionManager;
		funcMgr.SetFunctionToAddress(shared::base + 0x129EE48, shared::base + 0x8104B0);
		funcMgr.SetFunctionToAddress(shared::base + 0x129EE50, shared::base + 0x8104B0);
		funcMgr.SetFunctionToAddress(shared::base + 0x12492C0, shared::base + 0x791CC0);
		funcMgr.SetFunctionToAddress(shared::base + 0x12A21AC, shared::base + 0x791CC0);
		funcMgr.SetFunctionToAddress(shared::base + 0x129EAD0, shared::base + 0x8064C0);
		funcMgr.SetFunctionToAddress(shared::base + 0x129CA1C, shared::base + 0x7E6E90);
		funcMgr.SetFunctionToAddress(shared::base + 0x129EBB4, shared::base + 0x7E6E90);
	}

	void ProcessSamEvents(Pl0000* player) noexcept
	{
		if (!player || !shared::base) return;

		bool isEnabled = g_GameStateManager.IsMainSamPlayer;
		GameFunctionManager& funcMgr = g_GameFunctionManager;

		if (!isEnabled) {
			RestoreRaidenRoutes();
			return;
		}

		uint32_t currentPhase = PhaseManager::ms_Instance.getCurrentSubPhase();
		uint32_t& playerAction = player->m_Rno0;
		uint32_t eventTrigger = *(uint32_t*)(shared::base + 0x14A9F04);
		bool isSpecialPhaseValue = IsSpecialPhase(currentPhase);
		bool isBtlSam3 = g_GameFunctionManager.CheckPhase("btl_sam_3");
		bool isNinjaRunEvent = g_GameFunctionManager.IsNinjaRunEvent();

		funcMgr.SetFunctionToAddress(shared::base + 0x129EBD0, shared::base + 0x78C900);

		uintptr_t funcAddress = isSpecialPhaseValue ? shared::base + 0x6C0B00 : shared::base + 0x6C3920;
		funcMgr.SetFunctionToAddress(shared::base + 0x129EDE0, funcAddress);

		uint32_t bossAction = GetBossAction();

		if (currentPhase == 0x380)
		{
			if (eventTrigger && player->field_3458 && bossAction != 393219)
				player->setRno(0, 1, 0, 0);
		}

		if (player->field_1370.m_TargetHandle.getEntity())
		{
			Entity* targetEntity = player->field_1370.m_TargetHandle.getEntity();
			Behavior* targetBehavior = targetEntity->m_pBehavior;

			if (targetBehavior)
			{
				BOOL em0080 = targetBehavior->getContext().hasInheritance(&Em0080::m_Context);
				BOOL em0200 = targetBehavior->getContext().hasInheritance(&Em0200::m_Context);
				BOOL em0600 = targetBehavior->getContext().hasInheritance(&Em0600::m_Context);

				if (playerAction == 0xE7 && targetBehavior->m_Rno0 == 0x90002 &&
					targetBehavior->m_Rno1 == 5 && em0080)
					((void(__thiscall*)(Behavior*))(shared::base + 0x92490))(targetBehavior);

				if ((em0080 || em0200 || em0600) && eventTrigger && player->field_3458 && bossAction != 393219)
					player->setRno(0, 1, 0, 0);
			}
		}



		bool useSamUpdate = !player->isUnarmed() && !eventTrigger;
		bool isQTEEnabled = currentPhase == 0x170 || currentPhase == 0x470 || currentPhase == 0x610;
		bool hasFullFuel = player->getFuelCapacity(1) == player->getFuelContainer();

		GameStateManager& stateMgr = g_GameStateManager;
		stateMgr.IsQTEForcedDisabled = isQTEEnabled;
		stateMgr.IsQTEEnabled = (isQTEEnabled && hasFullFuel) ? 1 : 0;

		bool isAttacking = (playerAction >= 0x100000) ||
			((player->m_ButtonLightAttack & (player->m_CurrentInput.m_On | player->m_CurrentInput.m_Trig)) != 0) ||
			((player->m_ButtonHeavyAttack & (player->m_CurrentInput.m_On | player->m_CurrentInput.m_Trig)) != 0);

		if (isAttacking && !isBtlSam3 && !isNinjaRunEvent)
		{
			// Attack active: Use Sam's Input Scheme and StateMachineFactory (both Pl1400 and Pl0010 vtables)
			funcMgr.SetFunctionToAddress(shared::base + 0x129EE48, shared::base + 0x492EB0);
			funcMgr.SetFunctionToAddress(shared::base + 0x129EE50, shared::base + 0x493BD0);
			funcMgr.SetFunctionToAddress(shared::base + 0x12492C0, shared::base + 0x468E60);
			funcMgr.SetFunctionToAddress(shared::base + 0x12A21AC, shared::base + 0x468E60);

			uintptr_t updateAddress = useSamUpdate ? shared::base + 0x45C700 : shared::base + 0x8064C0;
			funcMgr.SetFunctionToAddress(shared::base + 0x129EAD0, updateAddress);

			// Murasama red slash attack effect
			funcMgr.SetFunctionToAddress(shared::base + 0x129CA1C, shared::base + 0x46BC60);
			funcMgr.SetFunctionToAddress(shared::base + 0x129EBB4, shared::base + 0x46BC60);
		}
		else
		{
			// Locomotion (idle, walk, run, sprint, ninja run): Use Raiden's native routes
			ResetFlagOnce = false;
			uintptr_t raidenInput = shared::base + 0x8104B0;
			funcMgr.SetFunctionToAddress(shared::base + 0x129EE48, raidenInput);
			funcMgr.SetFunctionToAddress(shared::base + 0x129EE50, raidenInput);
			funcMgr.SetFunctionToAddress(shared::base + 0x129EAD0, shared::base + 0x8064C0);
			funcMgr.SetFunctionToAddress(shared::base + 0x012492C0, shared::base + 0x791CC0);
			funcMgr.SetFunctionToAddress(shared::base + 0x012A21AC, shared::base + 0x791CC0);

			// Retain Murasama effect routing while Sam moveset is toggled on
			funcMgr.SetFunctionToAddress(shared::base + 0x129CA1C, shared::base + 0x46BC60);
			funcMgr.SetFunctionToAddress(shared::base + 0x129EBB4, shared::base + 0x46BC60);
		}

		if (isBtlSam3)
		{
			if ((playerAction != 0x100051 && playerAction != 0x10004E) && playerAction >= 0x100000)
				player->setRno(0, 0, 0, 0);
			else if (playerAction == 0x100051 || playerAction == 0x10004E)
				player->setRno(0x201, 0, 0, 0);
		}
	}
};

inline EventTriggerHandle* g_EventTriggerHandle = &EventTriggerHandle::GetInstance();
