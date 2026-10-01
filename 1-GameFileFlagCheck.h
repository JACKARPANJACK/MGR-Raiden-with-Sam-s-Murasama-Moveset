#pragma once
#include "1-GameFunctionManager.h"

class BxmFlagHandle
{
private:
	inline static bool Flag53;
	inline static bool Flag53_;

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
	}

	int CheckSheath_2097152Flag0(Pl0000* player)
	{
		if (player->isUnarmed())
			return 0;

		((void(__thiscall*)(Pl0000*))(shared::base + 0x7E9130))(player);

		bool hasFlag53 = player->ckSeqFlag(53);

		if (hasFlag53 && !Flag53)
		{
			player->m_bSwordHidden = 1;
			player->removeConstraint(CONSTRAINT_4);

			Entity* sheathEntity = player->m_pSheathEntity;
			Entity* swordEntity = player->m_SwordHandle.getEntity();

			if (sheathEntity && swordEntity)
				player->attachObject(CONSTRAINT_4, sheathEntity, swordEntity, ATTACH_POINT_710, 0);

			Flag53 = true;
		}
		else if ((!hasFlag53 && Flag53) || player->m_OldRno0 == 0x100101)
		{
			player->m_bSwordHidden = 0;
			g_GameFunctionManager.UpdateSwordConstraints(player);
			Flag53 = false;
		}

		((void(__thiscall*)(Behavior*))(shared::base + 0x45D250))(player);
		return *(char*)((char*)player + FIELD_544C);
	}

	void CheckSheath_2097152Flag1(Behavior* behavior)
	{
		Pl0000* player = static_cast<Pl0000*>(behavior);

		if (Trigger::StaFlags.STA_QTE && !Flag53_ && g_GameStateManager.IsMainSamPlayer)
		{
			Entity* sheath = player->m_pSheathEntity;
			if (sheath && sheath->m_pBehavior)
				sheath->m_pBehavior->requestAnimationByMap(SHEATH_ANIMATION);
			Flag53_ = true;
		}

		if (!Trigger::StaFlags.STA_QTE)
			Flag53_ = false;

		if (player->isUnarmed())
			return;

		const int newField854 = player->ckSeqFlag(21) ? ATTACH_POINT_710 : ATTACH_POINT_710;
		*(int*)((char*)behavior + FIELD_5444) = newField854;

		if (newField854 != *(int*)((char*)behavior + FIELD_5448))
		{
			EntityHandle* entityHandle = (EntityHandle*)((char*)behavior + FIELD_FF4);
			Entity* attachedEntity = entityHandle->getEntity();

			if (attachedEntity)
			{
				*(int*)((char*)behavior + FIELD_5448) = newField854;
				behavior->removeConstraint(CONSTRAINT_17);

				if (newField854 == ATTACH_POINT_710)
					behavior->attachObject(CONSTRAINT_17, behavior->m_pEntity, attachedEntity, ATTACH_POINT_710, 10);
			}
		}

		((void(__thiscall*)(Behavior*))(shared::base + 0x45D250))(behavior);
	}

private:
	BxmFlagHandle() = default;
	~BxmFlagHandle() = default;
};

inline BxmFlagHandle& g_BxmFlagHandle = BxmFlagHandle::GetInstance();