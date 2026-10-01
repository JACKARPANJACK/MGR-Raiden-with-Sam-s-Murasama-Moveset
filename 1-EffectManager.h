#pragma once
#include "1-GameFunctionManager.h"
#include "1-GlobleVarious.h"
#include "1-ChangePlayerManager.h"

class EffectHandle
{
private:
	inline static bool b_SwordTailEffects = false;
	inline static bool b_RipperModeRainingEffect = false;

	static const uintptr_t EFFECT_ADDRESS_TABLE[3];
	static const uintptr_t TARGET_ADDRESSES[2];
	static constexpr unsigned int SPECIAL_PHASES[3] = { 0x170, 0x610, 0x470 };

public:
	[[nodiscard]] static auto GetInstance() -> EffectHandle&
	{
		static EffectHandle instance;
		return instance;
	}

	cEspControler m_RageEspController;
	cEspControler m_BladeModeEspController;
	cEspControler m_RainingEspController;
	cEspControler m_BossSamSwordEffectController;
	cEspControler m_BossSamRipperEffectController;
	cEspControler m_Boss0030Esp;
	cEspControler m_Boss0030Esp_;
	cEspControler m_RageEsp;

	void Reset_EffectHandle_StaticVariable()
	{
		b_SwordTailEffects = false;
		b_RipperModeRainingEffect = false;
	}

	void HandlePlayerEffects(Pl0000* pPlayer) noexcept
	{
		if (!pPlayer)
			return;

		bool bBladeModeActive = pPlayer->isBladeModeActive();
		if (bBladeModeActive && !b_SwordTailEffects)
		{
			pPlayer->createEffect(752, &m_BladeModeEspController);
			b_SwordTailEffects = true;
			if (pPlayer->m_pSheathEntity && pPlayer->m_pSheathEntity->m_pBehavior)
				pPlayer->m_pSheathEntity->m_pBehavior->requestAnimationByMap(5);
		}
		else if (!bBladeModeActive && b_SwordTailEffects)
		{
			b_SwordTailEffects = false;
			m_BladeModeEspController.FadeUnits(5.0f, 0.0f);
		}

		bool bRipperModeEnabled = (pPlayer->m_bRipperModeEnabled == 1);
		if (bRipperModeEnabled && !b_RipperModeRainingEffect)
		{
			pPlayer->createEffect(754, &m_RainingEspController);
			b_RipperModeRainingEffect = true;
		}
		else if (!bRipperModeEnabled && b_RipperModeRainingEffect)
		{
			m_RainingEspController.FadeUnits(5.0f, 0.0f);
			b_RipperModeRainingEffect = false;
		}
	}

	void HandleAttackEffects(Pl0000* pPlayer, int nCurrentPhase, const char* pAnimationName) noexcept
	{
		if (!pPlayer)
			return;

		bool bBladeModeSpecial = (pPlayer->m_nBladeModeType >= 5 && pPlayer->m_nBladeModeType != 21);
		bool bInAir = pPlayer->isInAir();
		bool bBladeModeActive = pPlayer->isBladeModeActive();
		bool bSpecialAction = (pPlayer->m_Rno0 == 0x100007 || pPlayer->m_Rno0 >= 0x100080);

		int nEffectChoice = 0;
		if (bBladeModeSpecial)
			nEffectChoice = 1;
		else if (bInAir || bBladeModeActive || bSpecialAction)
			nEffectChoice = 2;
		else
			nEffectChoice = 3;

		if (nEffectChoice >= 1 && nEffectChoice <= 3)
		{
			uintptr_t pEffectAddress = EFFECT_ADDRESS_TABLE[nEffectChoice - 1];
			for (int i = 0; i < 2; ++i)
				g_GameFunctionManager.SetFunctionToAddress(TARGET_ADDRESSES[i], pEffectAddress);
		}
	}

	void HandleSlowRateRemoval(BehaviorEmBase* pEnemy) noexcept
	{
		if (!pEnemy)
			return;

		Pl0000* pPlayer = pEnemy->m_pEnemy;
		if (!pPlayer)
			return;

		unsigned int nCurrentPhase = PhaseManager::ms_Instance.getCurrentSubPhase();
		if (pPlayer->isBladeModeActive() || pPlayer->m_Rno0 == 0x100007 ||
			pPlayer->m_Rno0 == 0xF6 || pPlayer->m_Rno0 == 0x100066 ||
			pPlayer->m_Rno0 == 0x100067 || Trigger::StaFlags.STA_QTE)
			return;

		bool bIsSpecialPhase = false;
		for (int i = 0; i < 3; ++i)
			if (nCurrentPhase == SPECIAL_PHASES[i])
			{
				bIsSpecialPhase = true;
				break;
			}
		// 非特殊阶段直接恢复全局慢动作

		if (!bIsSpecialPhase)
		{
			if (g_RateMan.m_aSlowRateUnit[0].m_fSlowRate <= 0.1f)
				g_RateMan.m_aSlowRateUnit[0].m_fSlowRate = 1.0f;
			return;
		}

		eObjID nEnemyIndex = pEnemy->m_ModelIndex;
		int nEnemyHealth = pEnemy->m_Hp;
		int nEnemyMaxHealth = pEnemy->m_HpMax;
		unsigned int nEnemyCurrentAction = pEnemy->m_Rno0;
		bool bShouldResetSlowRate = false;

		switch (nCurrentPhase)
		{
		case 0x170:
			bShouldResetSlowRate = nEnemyIndex == 0x20110 && nEnemyCurrentAction != 0xE000A && nEnemyCurrentAction != 0xE0003;
			break;

		case 0x610:
			bShouldResetSlowRate = nEnemyMaxHealth > 0 && nEnemyIndex == 0x20020 && nEnemyCurrentAction != 0x3000C && nEnemyHealth >= nEnemyMaxHealth / 10;
			break;

		case 0x470:
			bShouldResetSlowRate = nEnemyIndex == 0x20310 && nEnemyCurrentAction != 0x3000D && nEnemyCurrentAction != 0xF0001;
			break;
		}

		if (bShouldResetSlowRate && g_RateMan.m_aSlowRateUnit[0].m_fSlowRate <= 0.1f)
			g_RateMan.m_aSlowRateUnit[0].m_fSlowRate = 1.0f;
	}

private:
	EffectHandle() = default;
	~EffectHandle() = default;
	EffectHandle(const EffectHandle&) = delete;
	EffectHandle(EffectHandle&&) = delete;
	auto operator=(const EffectHandle&) -> EffectHandle & = delete;
	auto operator=(EffectHandle&&) -> EffectHandle & = delete;
};

const uintptr_t EffectHandle::EFFECT_ADDRESS_TABLE[3] =
{
	shared::base + 0x7E6E90,
	shared::base + 0x46BC60,
	shared::base + 0x1EE70
};

const uintptr_t EffectHandle::TARGET_ADDRESSES[2] =
{
	shared::base + 0x129CA1C,
	shared::base + 0x129EBB4
};

inline auto* g_EffectHandle = &EffectHandle::GetInstance();