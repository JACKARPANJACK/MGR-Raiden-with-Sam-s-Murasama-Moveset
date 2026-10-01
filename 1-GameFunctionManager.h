#pragma once
#include "1-GlobleVarious.h"

class GameFunctionManager
{
public:
	static GameFunctionManager& GetInstance()
	{
		static GameFunctionManager instance;
		return instance;
	}

private:
	GameFunctionManager() = default;
	~GameFunctionManager() = default;
	GameFunctionManager(const GameFunctionManager&) = delete;
	GameFunctionManager& operator=(const GameFunctionManager&) = delete;

	static const int chargesAnimationId[5];
	static int originalValues[136];
	static bool valuesSaved;
	static bool once;

public:
	void SetFunctionToAddress(unsigned int addr1, unsigned int addr2)
	{
		injector::WriteMemory<unsigned int>(addr1, addr2, true);
	}

	int HexToInt(const std::string& str)
	{
		if (str.length() > 4) return 0;

		unsigned int hex_num = 0;
		for (size_t i = 0; i < str.length(); ++i)
			hex_num = (hex_num << 8) | static_cast<unsigned char>(str[i]);

		return ((hex_num & 0xFF000000) >> 24) | ((hex_num & 0x00FF0000) >> 8)
			| ((hex_num & 0x0000FF00) << 8) | ((hex_num & 0x000000FF) << 24);
	}

	int StringHash32(const char* str)
	{
		if (!str) return 0;
		return ((int(__cdecl*)(const char*))(shared::base + 0xA03EA0))(str);
	}

	bool CheckPhase(const char* phaseName)
	{
		return PhaseManager::ms_Instance.isCurrentPhase(phaseName);
	}

	bool CanActiveRipper(Pl0000* player)
	{
		if (!player) return false;
		return player->getFuelContainer() == player->getFuelCapacity(1) && !Trigger::StaFlags.STA_QTE;
	}

	bool IsNinjaRunEvent()
	{
		return Trigger::GameFlags.GAME_SLIDER_NINJARUN_MODE ||
			Trigger::GameFlags.GAME_MISSILE_NINJYARUN_MODE ||
			Trigger::StaFlags.STA_NINJARUN;
	}

	void PlayEvent(const char* event)
	{
		if (!event) return;
		((void(__cdecl*)(const char*))(shared::base + 0xA5E1B0))(event);
	}

	int Play_Sound(const char* sound, void* param, int i1, int i2)
	{
		if (!sound) return 0;
		return ((int(__cdecl*)(const char*, void*, int, int))(shared::base + 0xA5E0C0))(sound, param, i1, i2);
	}

	int NextMission(PhaseManager* manager, const char* mission, int i1, int i2)
	{
		if (!manager || !mission) return 0;
		return ((int(__thiscall*)(PhaseManager*, const char*, int, int))(shared::base + 0x95EA40))(manager, mission, i1, i2);
	}

	void AdjustPlayerRotation(Pl0000* player)
	{
		if (!player) return;
		((void(__thiscall*)(Pl0000*))(shared::base + 0x7884C0))(player);
		((void(__thiscall*)(Pl0000*, int))(shared::base + 0x786010))(player, 1);
		((void* (__thiscall*)(Pl0000*, float, float, float, float))(shared::base + 0x77B270))(player, 1.0f, 0.00017453292f, 3.1415926f, 0.0f);
	}

	void RemoveCollision(Pl0000* player, bool enableCollision, bool disableCollision)
	{
		if (!player) return;

		CharacterControl* playerControl = player->m_pCharacterControl;
		if (!playerControl) return;

		CharacterControl* enemyControl = nullptr;
		Entity* targetEntity = player->field_1370.m_TargetHandle.getEntity();
		if (targetEntity && targetEntity->m_pBehavior)
			enemyControl = targetEntity->m_pBehavior->m_pCharacterControl;

		if (enableCollision && !disableCollision)
		{
			((float* (__thiscall*)(CharacterControl*))(shared::base + 0x4E3C10))(playerControl);
			if (enemyControl)
				((float* (__thiscall*)(CharacterControl*))(shared::base + 0x4E3C10))(enemyControl);
			player->field_314 = 0;
			player->field_318 = 0;
		}
		else if (!enableCollision && disableCollision)
		{
			((float* (__thiscall*)(CharacterControl*))(shared::base + 0x4E6D00))(playerControl);
			if (enemyControl)
				((float* (__thiscall*)(CharacterControl*))(shared::base + 0x4E6D00))(enemyControl);
		}
	}

	void ResetPlayerState(Pl0000* player)
	{
		if (!player) return;

		player->m_Rot.x = 0.0f;
		((cParts * (__thiscall*)(cModelBase*))(shared::base + 0x7E8AA0))(player);

		player->field_3E60 = { 0.0f, 0.0f, 0.0f, 1.0f };
		player->field_3E70 = 0.0f;
		player->field_3E74 = 0.0f;
		player->field_3E78 = 0.0f;
		player->m_TransSpeed = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);
		player->field_1020 = Hw::cVec4(0.0f, 0.0f, 0.0f, 0.0f);

		Entity* targetEntity = player->field_1370.m_TargetHandle.getEntity();
		if (targetEntity)
		{
			((void(__thiscall*)(int, Entity*, float*, int))(shared::base + 0x77B380))(reinterpret_cast<int>(player), targetEntity, &player->m_LocalMatrix._41, 1);
			ResetQTE(player, -1, targetEntity);
			((void(__thiscall*)(Animation*, int))(shared::base + 0x4B90))(player->m_pAnimation, 0);
		}
	}

	void PressXButtonEffect(Pl0000* player)
	{
		if (!player) return;

		uint32_t eventTrigger = *(uint32_t*)(shared::base + 0x14A9F04);

		if (Trigger::StaFlags.STA_QTE || player->m_Rno0 == 0x100059)
			injector::WriteMemory(shared::base + 0x14B56B4, 1);
		else if (eventTrigger)
		{
			if (((int(__cdecl*)(Hw::eSaveKeybind))(shared::base + 0x61D2D0))(Hw::KEYBIND_EXECUTION))
				injector::WriteMemory(shared::base + 0x14B56B4, 1);
			else
				injector::WriteMemory(shared::base + 0x14B56B4, 0);
		}

		if (!eventTrigger)
			injector::WriteMemory(shared::base + 0x14B56B4, 0);
	}

	void IsKeyPressXAndEnableFinishQTE(Pl0000* player)
	{
		if (!player) return;

		if (Hw::KEYBOARD_MAP::KB_X && player->m_Rno0 != 0x100066 && player->m_Rno0 != 0x100067)
		{
			AdjustPlayerRotation(player);
			player->m_bSwordHidden = false;
			player->setRno(0x100067, 0, 0, 0);
		}
	}

	void UpdateSwordConstraints(Pl0000* player)
	{
		if (!player || !player->field_13F8 || g_GameStateManager.IsRoundTripActive)
			return;

		Entity* swordEntity = player->m_SwordHandle.getEntity();
		if (!swordEntity)
		{
			player->field_13F8 = 1;
			return;
		}

		Entity* constraintsEntity = player->getConstraintsEntity(3);
		if (!constraintsEntity)
			return;

		cParts* handBone = player->getPartsPtrNo(0x720u);
		if (!handBone)
			return;

		constexpr float epsilon = 0.0099999998f;
		const float xPos = handBone->m_TransPos.x;
		int handAttachState = player->field_13F4;

		if (xPos <= -epsilon && fabs(xPos) < epsilon && fabs(handBone->m_TransPos.y) < epsilon)
			handAttachState = 0;

		if (xPos < -0.089999996f)
			handAttachState = 1;

		if (handAttachState != 0)
		{
			if (constraintsEntity->m_pBehavior)
				constraintsEntity->m_pBehavior->removeConstraint(4);

			if (player->field_13F4)
				player->removeConstraint(4);

			player->field_13F4 = handAttachState;

			switch (handAttachState)
			{
			case 0:
				if (constraintsEntity->m_pBehavior)
					constraintsEntity->m_pBehavior->attachObject(4, constraintsEntity, swordEntity, 1808, 0);
				break;
			case 1:
				player->attachObject(4, player->m_pEntity, swordEntity, 1792, 0);
				break;
			}

			if (swordEntity->m_pBehavior)
			{
				Pl0012* swordInstance = reinterpret_cast<Pl0012*>(swordEntity->m_pBehavior);
				if (handAttachState >= 1 && handAttachState <= 3)
					((char* (__thiscall*)(int))(shared::base + 0x811BE0))(reinterpret_cast<int>(swordInstance));
				else
					((char* (__thiscall*)(int))(shared::base + 0x811AC0))(reinterpret_cast<int>(swordInstance));
			}
		}
	}

	void PlayAnimationWithSpeed(Behavior* behavior, float speed1, float speed2)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*, float, float))(shared::base + 0x794790))(behavior, speed1, speed2);
	}

	bool IsAnimationEnded(Behavior* behavior, int param = 0)
	{
		if (!behavior) return true;
		return ((bool(__thiscall*)(Behavior*, int))(shared::base + 0x694CE0))(behavior, param);
	}

	void CustomActionFromAnimation(Pl0000* player, int animationMapID, bool useActionName, const char* actionName = nullptr)
	{
		if (!player) return;

		switch (player->m_Rno1)
		{
		case 0:
			AdjustPlayerRotation(player);
			player->field_940 = 0;
			BladeModeFadeOut(player);
			player->m_SpeedRate;
			((cParts * (__thiscall*)(Pl0000*))(shared::base + 0x794FC0))(player);

			if (!useActionName)
				player->requestAnimationByMap(animationMapID);
			else if (actionName)
				player->requestAnimationByName(actionName, 0, 0.0f, 1, 0, 0.0f, 1.0f);

			++player->m_Rno1;
			player->field_26D4 = 0;
			player->m_bSwordHidden = player->isUnarmed() ? 1 : 0;
			break;

		case 1:
			player->field_11A4 = 1;
			PlayAnimationWithSpeed(player, 1.0f, 1.0f);

			if (IsAnimationEnded(player))
				player->setIdle(0);
			break;

		default:
			break;
		}
	}

	void CustomThreeStepAnimation(Pl0000* player, int anim1, int anim2, int anim3)
	{
		if (!player) return;

		struct StateAction
		{
			bool shouldRequestAnim;
			int animationId;
			int nextState;
		} stateActions[] = {
			{true,  anim1, 1},
			{false, 0,     2},
			{true,  anim2, 3},
			{false, 0,     4},
			{true,  anim3, 5},
			{false, 0,     0}
		};

		int state = player->m_Rno1;

		if (state < 0 || state > 5)
			return;

		sub_6C80A0(player, 1.0f, 1.0f);

		if (state == 5)
		{
			if (IsAnimationEnded(player, 0))
				player->setIdle(0);
			return;
		}

		struct StateAction* action = &stateActions[state];

		if (action->shouldRequestAnim)
		{
			if (state == 0)
			{
				AdjustPlayerRotation(player);
				player->m_bSwordHidden = 0;
			}
			player->requestAnimationByMap(action->animationId);
			player->m_Rno1 = action->nextState;
		}
		else
		{
			if (IsAnimationEnded(player, 0))
				player->m_Rno1 = action->nextState;
		}
	}

	void InstantCharges(unsigned int pAnimUnit)
	{
		unsigned int animationName = injector::ReadMemory<unsigned int>(pAnimUnit, true);
		animationName = injector::ReadMemory<unsigned int>(animationName + 0x4, true);

		if (g_GameStateManager.InstantCharge && !once)
		{
			for (unsigned int j = 0, i = 0; i <= 135; ++i, j += 0x3C)
			{
				const int targetAnim = injector::ReadMemory<int>(animationName + j, true);

				for (int k = 0; k < 5; ++k)
				{
					if (targetAnim == chargesAnimationId[k] &&
						injector::ReadMemory<int>(animationName + 0xC + j, true) > 1)
					{
						originalValues[i] = injector::ReadMemory<int>(animationName + 0xC + j, true);
						SetFunctionToAddress(animationName + 0xC + j, 1);
						valuesSaved = true;
						break;
					}
				}
			}
			once = true;
		}
		else if (!g_GameStateManager.InstantCharge && once)
		{
			if (valuesSaved)
			{
				for (unsigned int j = 0, i = 0; i <= 135; ++i, j += 0x3C)
				{
					const int targetAnim = injector::ReadMemory<int>(animationName + j, true);

					for (int k = 0; k < 5; ++k)
					{
						if (targetAnim == chargesAnimationId[k] && originalValues[i] > 0)
						{
							SetFunctionToAddress(animationName + 0xC + j, originalValues[i]);
							originalValues[i] = 0;
							break;
						}
					}
				}
				valuesSaved = false;
			}
			once = false;
		}
	}

	void ResetQTE(Behavior* behavior, int param, Entity* entity)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*, int, Entity*))(shared::base + 0x7F5690))(behavior, param, entity);
	}

	void FixQTECamera(bool fixQTE)
	{
		static bool isFixed = false;

		if (fixQTE && !isFixed)
		{
			g_GameCamera.field_690 = 1;
			SetCameraSmoothness(&g_GameCamera, 16.0f);
			injector::MakeNOP(shared::base + 0xA3A2E2, 2, true);
			isFixed = true;
		}
		else if (!fixQTE && isFixed)
		{
			injector::WriteMemoryRaw(shared::base + 0xA3A2E2, g_GameStateManager.OriginalBytes, 2, true);
			isFixed = false;
		}
	}

	void HandleQTECameraAndDamage(BehaviorEmBase* enemy)
	{
		Pl0000* player = enemy->m_pEnemy;

		if (!player) return;

		static const int qteActions[] = {
			0xF4, 0xF6, 0xF5, 0x100500, 0x100501,
			0x100502, 0x100203, 0x100504,0x100600
		};

		bool shouldFixCamera = false;
		for (int action : qteActions)
		{
			if (player->m_Rno0 == action)
			{
				shouldFixCamera = true;
				((int(__thiscall*)(int* a1, float a2, int a3, cCameraGame * a4))(shared::base + 0x9B3E80))(&g_GameCamera.field_580, 0.0, 0, &g_GameCamera);
				break;
			}
		}

		FixQTECamera(shouldFixCamera);

		if (shouldFixCamera)
		{
			int modelIndex = enemy->m_ModelIndex;

			bool isBossEnemy = (modelIndex == 0x20020 || modelIndex == 0x20110 || modelIndex == 0x20310);
			bool isNotPlayer = (modelIndex != 0x10010);

			Entity* entity = enemy->m_pEntity;

			if (isBossEnemy && isNotPlayer && player->ckSeqFlag(56))
			{
				int damage = g_GameStateManager.IsSamuraiWarningActive ? 50 : 100;
				enemy->damage(damage, 1);
			}

			if (entity->m_ObjId == 0x20040 || entity->m_ObjId == 0x2C040)
				enemy->setRno(0x0, 0, 0, 0);
		}
	}

	void SetCameraSmoothness(cCameraGame* camera, float smoothness)
	{
		if (!camera) return;
		((void(__thiscall*)(cCameraGame*, float))(shared::base + 0x9A8810))(camera, smoothness);
	}

	void IncreaseMovementDistance(Behavior* behavior, float dx, float dy, float dz)
	{
		if (!behavior || Trigger::StpFlags.STP_OBJ)
			return;

		behavior->m_Rot.x = std::clamp(behavior->m_Rot.x, -0.95993108f, 0.17453292f);

		float* field_370 = (float*)((char*)behavior + 0xBE0);
		field_370[0] = dx;
		field_370[1] = dy;
		field_370[2] = dz;

		D3DXMATRIX* localMatrix = (D3DXMATRIX*)((char*)behavior + 0x10);
		D3DXVec3TransformNormal((D3DXVECTOR3*)field_370, (D3DXVECTOR3*)field_370, localMatrix);

		behavior->m_TransPos.x += field_370[0];
		behavior->m_TransPos.y += field_370[1];
		behavior->m_TransPos.z += field_370[2];
		behavior->m_TransPos.w += field_370[3];

		*(int*)((char*)behavior + 0x11A4) = 1;

		float* matrix_12 = (float*)((char*)behavior + 0x924);
		float* boneIndex = (float*)((char*)behavior + 0x910);
		*matrix_12 -= *boneIndex;

		if (behavior->m_ModelIndex <= 0x11400)
		{
			((Behavior * (__thiscall*)(Pl1400*))(shared::base + 0x4645F0))(reinterpret_cast<Pl1400*>(behavior));
			((void(__thiscall*)(Pl0000*, int))(shared::base + 0x786010))(reinterpret_cast<Pl0000*>(behavior), 1);
		}
	}

	void ChangePlayerName(int playerId)
	{
		g_GameStateManager.SelectedPlayerIdentifier = playerId;

		const size_t nameAddress = 0x12B6CEF;
		char digits[3] = { 0 };

		if (playerId >= 10)
		{
			digits[0] = static_cast<char>(playerId / 10 + '0');
			digits[1] = static_cast<char>(playerId % 10 + '0');
		}
		else
		{
			digits[0] = '0';
			digits[1] = static_cast<char>(playerId + '0');
		}

		for (size_t i = 0; i < 2; ++i)
		{
			const size_t address = nameAddress + i;
			if (injector::ReadMemory<char>(shared::base + address, true) != digits[i])
				injector::WriteMemory<char>(shared::base + address, digits[i], true);
		}
	}

	void Disable46DF00_ByJump()
	{
		DWORD func_addr = shared::base + 0x0046DF00;
		DWORD early_exit_addr = shared::base + 0x0046E2EF;

		DWORD jmp_offset = early_exit_addr - (func_addr + 5);

		injector::WriteMemory<BYTE>(func_addr, 0xE9, true);
		injector::WriteMemory<DWORD>(func_addr + 1, jmp_offset, true);
	}

	void CreateEffect(Behavior* behavior, int id, cEspControler* esp)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*, int, cEspControler*))(shared::base + 0x7C3470))(behavior, id, esp);
	}

	void sub_480160(Pl0000* player)
	{
		if (!player) return;
		((void(__thiscall*)(Pl0000*))(shared::base + 0x480160))(player);
	}

	void BladeModeFadeOut(Pl0000* player)
	{
		if (!player) return;
		((void* (__thiscall*)(Pl0000*))(shared::base + 0x77AA80))(player);
	}

	void FuelGaugeGone(Pl0000* player, float value)
	{
		if (!player) return;
		((void(__thiscall*)(Pl0000*, float))(shared::base + 0x7C3000))(player, value);
	}

	Hw::cVec4 GetPositionFromMatrix(cParts* part)
	{
		if (!part) return Hw::cVec4();
		return *(Hw::cVec4*)&part->m_LocalMatrix._41;
	}

	void sub_47C660(Behavior* behavior)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*))(shared::base + 0x47C660))(behavior);
	}

	void sub_47DDC0(Behavior* behavior)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*))(shared::base + 0x47DDC0))(behavior);
	}

	void sub_47FFB0(Behavior* behavior)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*))(shared::base + 0x47FFB0))(behavior);
	}

	void sub_6C80A0(Behavior* behavior, float f1, float f2)
	{
		if (!behavior) return;
		((void(__thiscall*)(Behavior*, float, float))(shared::base + 0x6C80A0))(behavior, f1, f2);
	}

	int sub_20B80(Em0020* a1, unsigned int a2, int _40, int a3, int a4, int a5)
	{
		if (!a1) return 0;
		return ((int(__thiscall*)(Em0020*, unsigned int, int, int, int, int))(shared::base + 0x20B80))(a1, a2, _40, a3, a4, a5);
	}

	BOOL hasAnyFCLeft(Pl0000* a1, bool bIgnoreUnused)
	{
		return ((BOOL(__thiscall*)(Pl0000 * a1, bool bIgnoreUnused))(shared::base + 0x7C3230))(a1, bIgnoreUnused);
	}
};

const int GameFunctionManager::chargesAnimationId[5] = { 82, 88, 93, 98, 135 };
int GameFunctionManager::originalValues[136] = { 0 };
bool GameFunctionManager::valuesSaved = false;
bool GameFunctionManager::once = false;

inline GameFunctionManager& g_GameFunctionManager = GameFunctionManager::GetInstance();