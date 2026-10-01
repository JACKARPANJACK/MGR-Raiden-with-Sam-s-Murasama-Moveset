#pragma once

// C++׼ͷļ
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// Windows SDK ͷļ
#include <Windows.h>
#include <d3dx9.h>

// ͷļ
// removed

// Ϸͷļ
#include <common.h>
#include <Hw.h>
#include <shared.h>

// Ϸͷļĸ˳
#include <Animation.h>
#include <Behavior.h>
#include <BehaviorBulletBase.h>
#include <BehaviorEmBase.h>
#include <cCameraGame.h>
#include <cGameUIManager.h>
#include <cPlayerInfoManager.h>
#include <cQTEButton.h>
#include <cSlowRateManager.h>
#include <cUISystem.h>
#include <CharacterControl.h>
#include <Em0010.h>
#include <Em0020.h>
#include <Em0030.h>
#include <Em0040.h>
#include <Em0060.h>
#include <Em0070.h>
#include <Em0080.h>
#include <Em0100.h>
#include <Em0200.h>
#include <Em0600.h>
#include <Em0110.h>
#include <Em01a0.h>
#include <Em014a.h>
#include <Em0220.h>
#include <Em0310.h>
#include <Em0700.h>
#include <EmBaseDLC.h>
// removed
// removed
#include <GameWorkManagerImplement.h>
#include <PhaseManager.h>
#include <Pl0000.h>
#include <Pl0012.h>
#include <Pl1400.h>
#include <PlayerManagerImplement.h>
#include <Trigger.h>
#include <VoiceSubtitleManagerImplement.h>

// Զͷļ
// removed
// removed

class GameStateManager
{
public:
	static GameStateManager& GetInstance()
	{
		static GameStateManager instance;
		return instance;
	}

	bool IsMainSamPlayer = false;
	bool IsDLCSamPlayer = false;
	bool InstantCharge = false;
	bool IsDLCPhaseActive = false;

	bool IsStyleChanged = false;
	bool IsRoundTripActive = false;
	int SelectedPlayerIdentifier = 0;

	bool IsQTEForcedDisabled = false;
	bool IsQTEEnabled = false;
	bool IsSamuraiWarningActive = false;

	bool ShouldDisplayGUI = false;

	uint8_t OriginalBytes[2] = { 0xA8, 0x01 };

	// Ϸ״̬־
	void ResetGameStates()
	{
		// These are runtime ownership flags, not persistent settings. Leaving
		// either one set makes the Sam hook look enabled after a phase/player
		// reset, which is unsafe for the Raiden campaign.
		IsMainSamPlayer = false;
		IsDLCSamPlayer = false;
		InstantCharge = false;
		IsDLCPhaseActive = false;
		IsStyleChanged = false;
		IsRoundTripActive = false;
		IsQTEForcedDisabled = false;
		IsQTEEnabled = false;
		IsSamuraiWarningActive = false;
		ShouldDisplayGUI = false;
	}

	// ȫ״̬
	void CompleteReset()
	{
		ResetGameStates();
		SelectedPlayerIdentifier = 0;
	}

private:
	GameStateManager() = default;
	~GameStateManager() = default;

	// ɾ캯͸ֵȷ
	GameStateManager(const GameStateManager&) = delete;
	GameStateManager& operator=(const GameStateManager&) = delete;
};

inline GameStateManager& g_GameStateManager = GameStateManager::GetInstance();

#include <cGame.h>

namespace Trigger {
    inline staFlags& StaFlags = g_StaFlags;
    inline gameFlags& GameFlags = g_GameFlags;
    inline stpFlags& StpFlags = g_StpFlags;
}

namespace Hw {
    enum eSaveKeybind
    {
        KEYBIND_NONE = -1,
        KEYBIND_MOVE_FORWARD = 0,
        KEYBIND_MOVE_BACK,
        KEYBIND_MOVE_LEFT,
        KEYBIND_MOVE_RIGHT,
        KEYBIND_JUMP,
        KEYBIND_LIGHT_ATTACK,
        KEYBIND_HEAVY_ATTACK,
        KEYBIND_BLADEMODE,
        KEYBIND_NINJARUN,
        KEYBIND_ACTION,
        KEYBIND_RIPPERMODE,
        KEYBIND_SWITCH_LOCK_ON,
        KEYBIND_USE_SUBWEAPON,
        KEYBIND_USE_ITEM,
        KEYBIND_AR_MODE,
        KEYBIND_WEAPON_SELECT_SCREEN,
        KEYBIND_CODEC_SCREEN,
        KEYBIND_PAUSE,
        KEYBIND_CAMERA_RESET,
        KEYBIND_EXECUTION,
        KEYBIND_DEFFENSIVE_OFFENSIVE,
        KEYBIND_FIRE_SUBWEAPON,
        KEYBIND_TOTAL
    };
}

#define m_nButtonsPressed m_On
#define m_fSlowRate m_SlowRate

inline D3DXVECTOR3* MyD3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
    float x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31;
    float y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32;
    float z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33;
    pOut->x = x;
    pOut->y = y;
    pOut->z = z;
    return pOut;
}
#define D3DXVec3TransformNormal MyD3DXVec3TransformNormal
