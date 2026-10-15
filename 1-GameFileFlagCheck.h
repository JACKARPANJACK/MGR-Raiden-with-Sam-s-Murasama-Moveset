#pragma once
#include "1-GameFunctionManager.h"
#include "SamResourceManager.h"
#include "SamSheathPolicy.h"

class BxmFlagHandle
{
private:
	inline static bool Flag53;
	inline static bool Flag53_;
	Pl0000* m_player = nullptr;
	Entity* m_sheath = nullptr;
	char m_animation[5]{};
	int m_swordHidden = -1;
	void Track(Pl0000* player) {
		if (m_player == player && m_sheath == player->m_pSheathEntity) return;
		Reset_BxmFlagHandle_StaticVariable();
		m_player = player; m_sheath = player->m_pSheathEntity;
	}

	enum FieldOffsets
	{
		FIELD_5444 = 0x5444,
		FIELD_5448 = 0x5448,
		FIELD_544C = 0x544C,
		FIELD_FF4 = 0xFF4
	};

	enum ConstraintIds
	{
		CONSTRAINT_4 = 4,
		CONSTRAINT_17 = 17
	};

	enum AnimationValues
	{
		ATTACH_POINT_1793 = 1793,
		ATTACH_POINT_2032 = 2032,
		ATTACH_POINT_710 = 0x710,
		SHEATH_ANIMATION = 5
	};

public:
	static BxmFlagHandle& GetInstance()
	{
		static BxmFlagHandle instance;
		return instance;
	}

	void Reset_BxmFlagHandle_StaticVariable()
	{
		Flag53 = 0;
		Flag53_ = 0;
		m_player = nullptr; m_sheath = nullptr;
		m_animation[0] = 0; m_swordHidden = -1;
	}

	int CheckSheath_2097152Flag0(Pl0000* player, bool chargePose = false)
	{
		if (!player) return 0;
		Track(player);
		if (player->isUnarmed() || (player->m_CustomWeaponBone == 0x701 && player->m_CustomWeaponHandle.getEntity()))
			return 0;

		bool hasFlag53 = chargePose || player->ckSeqFlag(53);

		if (hasFlag53 && (!Flag53 || !player->getConstraintById(CONSTRAINT_4)))
		{
			Entity* sheathEntity = player->m_pSheathEntity;
			Entity* swordEntity = player->m_pBladeEntity ? player->m_pBladeEntity : player->m_SwordHandle.getEntity();

			if (sheathEntity && reinterpret_cast<uintptr_t>(sheathEntity) >= 0x10000 &&
				swordEntity && reinterpret_cast<uintptr_t>(swordEntity) >= 0x10000)
			{
                player->m_bSwordHidden = 1;
                g_GameFunctionManager.ParkSword(player);
				Flag53 = true;
			}
		}
		else if ((!hasFlag53 && Flag53) || player->m_OldRno0 == 0x100101)
		{
			player->m_bSwordHidden = 0;
			g_GameFunctionManager.UpdateSwordConstraints(player);
			Flag53 = false;
		}

		return *(char*)((char*)player + FIELD_544C);
	}

	void CheckSheath_2097152Flag1(Behavior* behavior, bool chargePose = false)
	{
		if (!behavior) return;
		Pl0000* player = static_cast<Pl0000*>(behavior);
		Track(player);

		if (Trigger::StaFlags.STA_QTE && !Flag53_ && g_GameStateManager.IsMainSamPlayer)
		{
			Entity* sheath = player->m_pSheathEntity;
			if (sheath && sheath->m_pBehavior)
				sheath->m_pBehavior->requestAnimationByMap(SHEATH_ANIMATION);
			Flag53_ = true;
		}

		if (!Trigger::StaFlags.STA_QTE || !g_GameStateManager.IsMainSamPlayer)
			Flag53_ = false;

		const bool holding = !player->isUnarmed() && (chargePose || player->ckSeqFlag(21));
		Constraints* hip = player->getConstraintById(3);
		const int newField854 = holding ? ATTACH_POINT_1793 : hip ? hip->m_nBone : ATTACH_POINT_2032;
		*(int*)((char*)behavior + FIELD_5444) = newField854;

		Constraints* grip = player->getConstraintById(CONSTRAINT_17);
		const bool matches = grip && grip->m_MainEntity.getEntity() == player->m_pEntity &&
			grip->m_ObjectEntityHandle.getEntity() == m_sheath &&
			grip->m_nBone == 0x701u && grip->m_nRotationBone == 10u;
		if (holding && m_sheath && reinterpret_cast<uintptr_t>(m_sheath) >= 0x10000 && !matches)
			SamSheathPolicy::BindGrip(player, m_sheath);
		else if (!holding && grip)
			player->removeConstraint(CONSTRAINT_17);
		*(int*)((char*)behavior + FIELD_5448) = newField854;
	}

	void UpdateSheathAnimation(Pl0000* player, const char* requested = nullptr, float playbackSpeed = 1.0f)
	{
		if (!player) return;
		Track(player);
		if (!m_sheath || !m_sheath->m_pBehavior || Flag53_) return;
		const char* name = requested;
		if (!name && player->m_pAnimationSlot && player->m_pAnimationSlot->m_pArray)
			name = player->m_pAnimationSlot->m_pArray[0].m_pAnimName;
		char code[5]{};
		if (!SamSheathPolicy::Code(code, name)) std::strcpy(code, "0000");
		// Grip/parking flag changes must not restart an in-progress charge clip.
		if (!std::strcmp(code, m_animation)) {m_swordHidden=player->m_bSwordHidden;return;}
		std::strcpy(m_animation, code);
		m_swordHidden = player->m_bSwordHidden;
		void* motion = SamResourceManager::Instance().GetSheathMotion(code);
		if (motion) m_sheath->m_pBehavior->setDirectAnimation(
			motion, nullptr, 0, 0.05f, 1.0f, 0x8000000, 0.0f, playbackSpeed);
		else m_sheath->m_pBehavior->requestAnimationByMap(player->m_bSwordHidden ? 4 : 5);
	}

private:
	BxmFlagHandle() = default;
	~BxmFlagHandle() = default;
};

inline BxmFlagHandle& g_BxmFlagHandle = BxmFlagHandle::GetInstance();
