#pragma once
#include "1-GameFunctionManager.h"
#include "1-ChangePlayerManager.h"
#include "1-GlobleVarious.h"

class PlayerActionHandler
{
public:
	inline static bool b_30025_1;
	inline static bool b_30025_2;
	inline static bool b_30025_3;
	inline static bool b_resetSheathAction;
	inline static bool b_resetPos_arm_1;
	inline static bool b_MistralFlag;

private:
	struct ActionContext {
		Pl0000* player = nullptr;
		Behavior* playerFace = nullptr;
		Behavior* PlayerSheath = nullptr;
		BehaviorEmBase* TargetEnemy = nullptr;
		Entity* TargetEnemyEntity = nullptr;
		std::string_view animationName;
		int Frame = 0;
		int currentSubPhase = 0;
		float TargetPlayerEnemyDistance = 0.0f;
		bool IsEnemyIndexEnableQTE = false;

		[[nodiscard]] Entity* HasValidEnemy() const { return TargetEnemyEntity; }

		[[nodiscard]] long long GetAnimCode() const {
			if (animationName.empty()) return 0;
			long long result = 0;
			for (char c : animationName) {
				if (c >= '0' && c <= '9') result = result * 10 + (c - '0');
				else break;
			}
			return result;
		}
	};

	using ActionHandler = void(*)(ActionContext&);
	static constexpr size_t ACTION_TABLE_SIZE = 0x102000;
	std::unique_ptr<ActionHandler[]> m_actionHandlers;

	static const inline std::unordered_set<uint32_t> s_idleActions = { 3, 4, 12, 79, 81, 91, 93, 97, 102, 105, 110, 111, 116, 117 };
	static const inline std::unordered_set<std::string_view> s_specialAnimations = { "9100", "9101", "9102", "9030", "9032", "9038", "9040", "9042", "9048", "9002", "9003", "9108" };
	static const inline std::unordered_set<uint32_t> s_customActions = { 0x100090, 0x100091, 0x100092, 0x100093, 0x100095, 0x100096, 0x100099, 0x100100, 0x100101, 0x100103, 0x100104, 0x100105, 0x100106, 0x100107, 0x100109 };
	static const inline std::unordered_set<unsigned int> s_forbiddenActions = { 0x10002B, 0x100007, 0x100064, 0x10000E, 0x10001D, 0x100018 };

	struct SheathAnimMaps {
		static const inline std::unordered_map<long long, int> s_sheathAnimMap100010 = { {2020,3010}, {2030,3010}, {2022,3016}, {2021,3017}, {2032,3015}, {2038,3014}, {2040,3020}, {2041,3022}, {2042,3024}, {2048,3024}, {2028,3018}, {2029,3018} };
		static const inline std::unordered_map<long long, int> s_sheathAnimMap10000F = { {2002,5}, {2003,5}, {2002,3060}, {2003,3040} };
	};

	static long long s_lastProcessedCode100010;
	static long long s_lastProcessedCode10000F;
	static float s_a1Value;

public:
	[[nodiscard]] static PlayerActionHandler& GetInstance()
	{
		static PlayerActionHandler instance;
		return instance;
	}

	void HandlePlayerActions(Pl0000* player)
	{
		if (!player) return;
		ActionContext ctx = CreateActionContext(player);
		uint32_t action = player->m_Rno0;
		if (action < ACTION_TABLE_SIZE && m_actionHandlers[action])
			m_actionHandlers[action](ctx);
	}

	[[nodiscard]] bool IsCustomAction(uint32_t action) const { return s_customActions.count(action); }

private:
	PlayerActionHandler() : m_actionHandlers(new ActionHandler[ACTION_TABLE_SIZE]()) { InitializeActionHandlers(); }
	~PlayerActionHandler() = default;
	PlayerActionHandler(const PlayerActionHandler&) = delete;
	PlayerActionHandler& operator=(const PlayerActionHandler&) = delete;
	PlayerActionHandler(PlayerActionHandler&&) = delete;
	PlayerActionHandler& operator=(PlayerActionHandler&&) = delete;

	void InitializeActionHandlers()
	{
		std::fill_n(m_actionHandlers.get(), ACTION_TABLE_SIZE, nullptr);
		RegisterBasicActions();
		RegisterQTEAndSpecialActions();
		RegisterCombatActions();
		RegisterCustomActions();
	}

	ActionContext CreateActionContext(Pl0000* player)
	{
		ActionContext ctx;
		ctx.player = player;
		ctx.currentSubPhase = PhaseManager::ms_Instance.getCurrentSubPhase();

		if (player) {
			if (player->m_FaceHandle.getEntity())
				ctx.playerFace = static_cast<Behavior*>(player->m_FaceHandle.getEntity()->m_pBehavior);
			if (player->m_pSheathEntity)
				ctx.PlayerSheath = static_cast<Behavior*>(player->m_pSheathEntity->m_pBehavior);
			if (player->m_pAnimationSlot)
				ctx.animationName = player->m_pAnimationSlot->m_pArray[0].m_pAnimName;
			ctx.Frame = player->m_AnimationFrame;

			Entity* targetEntity = player->field_1370.m_TargetHandle.getEntity();
			if (targetEntity) {
				ctx.TargetEnemyEntity = targetEntity;
				ctx.TargetEnemy = static_cast<BehaviorEmBase*>(targetEntity->m_pBehavior);
				ctx.TargetPlayerEnemyDistance = (ctx.TargetEnemy->m_TransPos - ctx.player->m_TransPos).length();
				ctx.IsEnemyIndexEnableQTE = (ctx.TargetEnemy != nullptr);
			}
		}

		return ctx;
	}

	void RegisterBasicActions()
	{
		m_actionHandlers[0x45] = [](ActionContext& ctx) {
			if ((ctx.animationName == "90f4" || ctx.animationName == "9954") &&
				GameFunctionManager::GetInstance().IsAnimationEnded(ctx.player, 0))
				ctx.player->setIdle(0);
		};
		m_actionHandlers[0xB] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100005, 0, 0, 0); };
		m_actionHandlers[0xC] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100006, 0, 0, 0); };
		m_actionHandlers[0x5B] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100017, 0, 0, 0); };
		m_actionHandlers[0x92] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100019, 0, 0, 0); };
		m_actionHandlers[0xCD] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100055, 0, 0, 0); };
		m_actionHandlers[0xCE] = [](ActionContext& ctx) { if (ctx.player) ctx.player->setRno(0x100056, 0, 0, 0); };
		m_actionHandlers[0x113] = [](ActionContext& ctx) {
			if (ctx.player && ctx.player->m_Rno1 < 2 && ctx.player->m_pSheathEntity && ctx.player->m_pBladeEntity) {
				ctx.player->removeConstraint(4);
				ctx.player->attachObject(4, ctx.player->m_pSheathEntity, ctx.player->m_pBladeEntity, 1808, 0);
			}
			};
		m_actionHandlers[0x3E] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 0: ctx.player->requestAnimationByName("2018", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			case 3: ctx.player->requestAnimationByName("201a", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			case 4: ctx.player->requestAnimationByName("201b", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			}
			};
		m_actionHandlers[0x10D] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 0: ctx.player->requestAnimationByMap(1406); break;
			case 2: ctx.player->requestAnimationByMap(1407); break;
			case 4: ctx.player->requestAnimationByMap(1408); break;
			}
			};
		m_actionHandlers[0x11A] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 0: ctx.player->requestAnimationByName("2660", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			case 2: ctx.player->requestAnimationByName("2661", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			case 4: ctx.player->requestAnimationByName("2662", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			}
			};
		m_actionHandlers[0xFF] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 0: ctx.player->requestAnimationByName("2362", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			case 2: ctx.player->requestAnimationByName("2363", 0, 0.0f, 1, 0, 0.0f, 1.0f); break;
			}
			};
		m_actionHandlers[0x134] = [](ActionContext& ctx) { Trigger::GameFlags.GAME_MISSILE_NINJYARUN_MODE = 0; };
	}

	void RegisterQTEAndSpecialActions()
	{
		m_actionHandlers[0x104] = [](ActionContext& ctx) {
			if (ctx.TargetEnemy && ctx.player && ctx.player->m_Rno0 == 0x104 && ctx.player->m_Rno1 == 1)
				ctx.TargetEnemy->m_TransPos = { 37.000f, 92.000f, -119.500f, 1.0f };
			};
		m_actionHandlers[0x10C] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && ctx.animationName == "2601" && gameFunctionManager.IsAnimationEnded(ctx.player, 0))
				ctx.player->setIdle(0);
			};
		m_actionHandlers[0x105] = [](ActionContext& ctx) {
			if (!ctx.player || !ctx.playerFace) return;
			if (ctx.animationName == "2532") {
				ctx.player->m_Rot.y = 4.709f;
				ctx.player->m_TransPos.z = -88.968f;
			}
			if (ctx.animationName == "253a")
				ctx.playerFace->requestAnimationByName("2e93", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			else if (ctx.animationName == "2530") {
				const char* faceAnim = (ctx.Frame >= 85.0f && ctx.Frame <= 110.0f) ? "2e8e" : "2e93";
				ctx.playerFace->requestAnimationByName(faceAnim, 0, 0.0f, 1, 0, 0.0f, 1.0f);
			}
			else if (ctx.animationName == "2531")
				ctx.playerFace->requestAnimationByName("2e82", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			};
		m_actionHandlers[0x131] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			if (ctx.player->m_AnimationFrame > 117.0f)
				ctx.player->removeConstraint(4);
			switch (ctx.player->m_Rno1) {
			case 1: case 2: b_resetPos_arm_1 = false; break;
			case 5:
				if (!b_resetPos_arm_1 && ctx.HasValidEnemy()) {
					if (g_pPlayerManager) g_pPlayerManager->setCustomWeaponEquipped(5);
					ctx.player->m_SwordState = 2;
					ctx.player->field_13F4 = -1;
					ctx.player->field_13F8 = 1;
					Hw::cVec4 pos{ -193.563f, -7.108f, 498.084f, 0.0f };
					ctx.player->m_TransPos = pos;
					ctx.TargetEnemy->m_TransPos = pos;
					b_resetPos_arm_1 = true;
				}
				break;
			case 7: b_resetPos_arm_1 = false; break;
			}
			};
		m_actionHandlers[0x12C] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 3:
				if (g_pPlayerManager) g_pPlayerManager->setCustomWeaponEquipped(5);
				ctx.player->m_SwordState = 2;
				ctx.player->field_13F4 = -1;
				ctx.player->field_13F8 = 1;
				break;
			case 5:
				if (ctx.Frame >= 30) ctx.player->removeConstraint(4);
				break;
			}
			};
		m_actionHandlers[0x120] = [](ActionContext& ctx) {
			if (!ctx.playerFace) return;
			if (ctx.animationName == "a353" || ctx.animationName == "a354" || ctx.animationName == "a35a" || ctx.animationName == "a35b")
				ctx.playerFace->requestAnimationByName("2e82", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			else if (ctx.animationName == "a359")
				ctx.playerFace->requestAnimationByName("2e8f", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			};
		m_actionHandlers[0x122] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.animationName == "a35e") {
				if (ctx.Frame >= 220.0f && ctx.Frame <= 221.0f && !b_MistralFlag && ctx.PlayerSheath) {
					ctx.PlayerSheath->requestAnimationByMap(69);
					b_MistralFlag = true;
				}
				else if (ctx.Frame >= 297.0f && ctx.Frame <= 299.0f && ctx.player) {
					gameFunctionManager.Play_Sound("pl1400_se_swd_scabbard_sheathe_s", ctx.player, -1, 0);
					Trigger::GameFlags.GAME_PLAYER_VISOR_ENABLED = 0;
				}
				else if (ctx.Frame >= 390.0f && ctx.PlayerSheath)
					ctx.PlayerSheath->requestAnimationByMap(4);
			}
			else b_MistralFlag = false;
			};
		m_actionHandlers[0x13F] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath || !ctx.playerFace) return;
			if (ctx.animationName == "994b") {
				ctx.player->place(Hw::cVec4(224.224f, 347.570f, -13.293f, 1.0f), Hw::cVec4(0.0f, 1.5f, 0.0f, 0.0f));
				ctx.player->requestAnimationByName("2ea2", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				ctx.PlayerSheath->requestAnimationByName("2ea2", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				ctx.playerFace->requestAnimationByName("2ea2", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				Trigger::GameFlags.GAME_PLAYER_VISOR_ENABLED = 0;
			}
			else if (ctx.animationName == "2ea2") {
				if (ctx.Frame >= 160.0f) {
					ctx.player->requestAnimationByName("2ea3", 0, 0.0f, 1, 0, 0.0f, 1.0f);
					ctx.PlayerSheath->requestAnimationByName("2ea3", 0, 0.0f, 1, 0, 0.0f, 1.0f);
					ctx.playerFace->requestAnimationByName("2ea3", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				}
				if (ctx.Frame >= 21.0f && ctx.Frame <= 22.0f)
					gameFunctionManager.Play_Sound("rc06_qte_ray_down", ctx.player, -1, 0);
			}
			else if (ctx.animationName == "2ea3" && ctx.player->m_AnimationFrame >= 182.0f) {
				ctx.player->requestAnimationByName("2ea4", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				ctx.PlayerSheath->requestAnimationByName("2ea4", 0, 0.0f, 1, 0, 0.0f, 1.0f);
				ctx.playerFace->requestAnimationByName("2ea4", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			}
			else if (ctx.animationName == "2ea4") {
				if (ctx.Frame <= 80.0f && ctx.player->m_pBladeEntity) {
					ctx.player->removeConstraint(4);
					ctx.player->setConstraintsBone(4, 1792, 0);
					ctx.player->attachObject(4, ctx.player->m_pEntity, ctx.player->m_pBladeEntity, 1792, 0);
				}
				if (ctx.Frame >= 500.0f)
					gameFunctionManager.NextMission(&PhaseManager::ms_Instance, "P470_Event", 1, 0);
			}
			};
		m_actionHandlers[0xC4] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			if (ctx.HasValidEnemy())
				ctx.player->m_Rot.y = ctx.TargetEnemy->m_Rot.y - 3.1415926f;
			if (ctx.player->m_Rno1 == 0)
				ctx.player->setTransPos({ -191.913f, -7.100f, 500.276f, 0.0f });
			};
	}

	void RegisterCombatActions()
	{
		m_actionHandlers[0x100010] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			long long code = ctx.GetAnimCode();
			if (code == 0) return;
			auto it = SheathAnimMaps::s_sheathAnimMap100010.find(code);
			if (it != SheathAnimMaps::s_sheathAnimMap100010.end()) {
				if (code != s_lastProcessedCode100010) {
					ctx.PlayerSheath->requestAnimationByMap(it->second);
					s_lastProcessedCode100010 = code;
				}
			}
			else s_lastProcessedCode100010 = 0;

			if (code == 2029 && ctx.Frame <= 2.0f && ctx.HasValidEnemy() && ctx.TargetPlayerEnemyDistance >= 2.0f)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 20.0f);
			else if (code == 2022 && ctx.Frame <= 23.0f && ctx.HasValidEnemy() && ctx.TargetPlayerEnemyDistance >= 2.0f)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 20.0f);
			else if (code == 2021 || code == 2028) {
				if (ctx.Frame <= 2) gameFunctionManager.AdjustPlayerRotation(ctx.player);
				if (ctx.Frame <= 43.0f) {
					if (ctx.HasValidEnemy()) {
						if (ctx.Frame <= 15.0f)
							gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 7.33333f);
						else {
							gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 30.0f);
							((D3DXVECTOR3 * (__thiscall*)(BehaviorAppBase*, Hw::cVec4*))(shared::base + 0x68E880))(ctx.player, &ctx.TargetEnemy->m_TransPos);
							ctx.player->adjustHeading(0.80000001f, 0.00017453292f, 0.08726646f, 0.0f);
							s_a1Value = (s_a1Value + 1.0f < 4.0f) ? (s_a1Value + 1.0f) : 4.0f;
							gameFunctionManager.sub_6C80A0(ctx.player, s_a1Value, 1.0);
						}
					}
					else gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, 0.48333333f);
				}
				else {
					s_a1Value = 1.0f;
					if (ctx.player->m_pAnimation) {
						ctx.player->m_pAnimation->field_E4 = 1.0f;
						ctx.player->m_pAnimation->field_E8 = 1.0f;
						ctx.player->m_pAnimation->field_EC = 1.0f;
					}
				}
				if (code == 2028 && ctx.Frame >= 220) {
					ctx.player->requestAnimationByMap(1348);
					if (ctx.PlayerSheath) ctx.PlayerSheath->requestAnimationByMap(5);
				}
			}
			};
		m_actionHandlers[0x100011] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && (ctx.animationName == "2108" || ctx.animationName == "2109") && ctx.Frame <= 26.0f && ctx.HasValidEnemy() && ctx.TargetPlayerEnemyDistance >= 2.0f)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 15.333333f);
			};
		m_actionHandlers[0x100012] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && (ctx.animationName == "2111" || ctx.animationName == "2111") && ctx.HasValidEnemy() && gameFunctionManager.IsAnimationEnded(ctx.player, 0) && ctx.TargetPlayerEnemyDistance >= 3.0f) {
				if (ctx.player->m_AnimationFrame <= 1.0f) gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, 1.5f);
				gameFunctionManager.Play_Sound("pl1400_se_swd_scrape", ctx.player, -1, 0);
				gameFunctionManager.Play_Sound("pl1400_se_swd_scrape_stop", ctx.player, -1, 0);
				ctx.player->setRno(0x100012, 2, 0, 0);
			}
			};
		m_actionHandlers[0x100014] = [](ActionContext& ctx) {
			if (ctx.PlayerSheath && ctx.animationName == "2200" && ctx.player->m_AnimationFrame >= 78.0f)
				ctx.PlayerSheath->requestAnimationByMap(5);
			};
		m_actionHandlers[0x10000F] = [](ActionContext& ctx) {
			if (!ctx.PlayerSheath) return;
			long long code = ctx.GetAnimCode();
			if (code == 0) return;
			auto it = SheathAnimMaps::s_sheathAnimMap10000F.find(code);
			if (it != SheathAnimMaps::s_sheathAnimMap10000F.end()) {
				if (code != s_lastProcessedCode10000F) {
					ctx.PlayerSheath->requestAnimationByMap(it->second);
					s_lastProcessedCode10000F = code;
				}
			}
			else s_lastProcessedCode10000F = 0;
			};
		m_actionHandlers[0x10001B] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			if (ctx.animationName == "24c0" || ctx.animationName == "24c0") {
				if (ctx.Frame <= 30.0f) gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, -0.059999995f);
			}
			else if (ctx.animationName == "24c1") {
				if (ctx.Frame >= 5.0f && ctx.Frame <= 23.0f) gameFunctionManager.IncreaseMovementDistance(ctx.player, -0.119999995f, 0.0f, 0.0f);
			}
			else if (ctx.animationName == "24c2") {
				if (ctx.Frame >= 5.0f && ctx.Frame <= 23.0f) gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.119999995f, 0.0f, 0.0f);
			}
			};
		m_actionHandlers[0x10001C] = [](ActionContext& ctx) {
			static auto& gameStateManager = GameStateManager::GetInstance();
			if (ctx.player && gameStateManager.IsStyleChanged && !ctx.player->m_bSwordHidden && ctx.player->m_AnimationFrame <= 1.0f)
				ctx.player->setRno(0x100097, 0, 0, 0);
			};
		m_actionHandlers[0x10001A] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && ctx.animationName == "2428" && ctx.TargetPlayerEnemyDistance >= 2.0f)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, ctx.TargetPlayerEnemyDistance / 20.0f);
			};
		m_actionHandlers[0x10001D] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			if (ctx.animationName == "2251") gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.15f, 0.0f);
			else if (ctx.animationName == "2252" && ctx.Frame >= 9.0f && ctx.Frame <= 30.0f)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, 0.339999995f);
			};
		m_actionHandlers[0x100018] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && ctx.player->m_Rno0 != 0x100105 && (ctx.animationName == "2411" || ctx.animationName == "2611"))
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, -0.55f, 0.0f);
			};
		m_actionHandlers[0x100019] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && (ctx.animationName == "2422" || ctx.animationName == "2623"))
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, -0.25f, 0.0f);
			else if (ctx.player && ctx.animationName == "2428")
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.0f, 0.109999995f);
			};
		m_actionHandlers[0x100076] = [](ActionContext& ctx) {
			if (!ctx.player || !ctx.player->m_pSheathEntity || !ctx.player->m_SwordHandle.getEntity()) return;
			if (Trigger::StaFlags.STA_QTE && ctx.currentSubPhase == 0xC60 && ctx.animationName == "2e91" && ctx.Frame >= 27.0f) {
				ctx.player->removeConstraint(4);
				ctx.player->attachObject(4, ctx.player->m_pSheathEntity, ctx.player->m_SwordHandle.getEntity(), 1808, 0);
			}
			};
		m_actionHandlers[0x100077] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.player->m_SheathHandle.getEntity()) return;
			auto* BehSheath = static_cast<Behavior*>(ctx.player->m_SheathHandle.getEntity()->m_pBehavior);
			if (!BehSheath) return;
			cParts* Bone = BehSheath->getPartsPtrNo(0x00Cu);
			cParts* Bone1 = BehSheath->getPartsPtrNo(0x00Bu);
			if (!Bone || !Bone1) return;
			if (ctx.player->m_Rno1 == 13) {
				Bone->m_TransPos = { 0.016f, 0.010f, 0.044f ,1.0f };
				Bone1->m_Rot.x = 0.0f;
				if (ctx.player->m_AnimationFrame > 474.0f) ctx.player->field_A00.FadeUnits(0.0f, 5.0f);
			}
			if (Trigger::StaFlags.STA_QTE && ctx.currentSubPhase == 0xC60 && ctx.animationName == "2e96") {
				if (ctx.Frame <= 12.0f && ctx.player->m_pSheathEntity && ctx.player->m_SwordHandle.getEntity()) {
					ctx.player->removeConstraint(4);
					ctx.player->attachObject(4, ctx.player->m_pSheathEntity, ctx.player->m_SwordHandle.getEntity(), 1808, 0);
				}
				else if (ctx.player) gameFunctionManager.UpdateSwordConstraints(ctx.player);
			}
			};
		m_actionHandlers[0x100082] = [](ActionContext& ctx) {
			if (ctx.playerFace) ctx.playerFace->requestAnimationByName((ctx.Frame <= 230.0f) ? "2e93" : "0000", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			};
	}

	void RegisterCustomActions()
	{
		m_actionHandlers[0x100090] = [](ActionContext& ctx) {
			static auto& gameStateManager = GameStateManager::GetInstance();
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			gameFunctionManager.CustomActionFromAnimation(ctx.player, 673, 0, "");
			ctx.PlayerSheath->requestAnimationByName("3030", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			if (gameStateManager.IsSamuraiWarningActive && ctx.player->m_AnimationFrame >= 46.0f && ctx.player->m_AnimationFrame <= 320.0f && ctx.TargetPlayerEnemyDistance <= 2.1f && ctx.IsEnemyIndexEnableQTE) {
				ctx.player->setRno(0x100502, 10, 0, 0);
				if (ctx.HasValidEnemy()) ctx.TargetEnemy->setRno(0x100502, 10, 0, 0);
			}
			};
		m_actionHandlers[0x100502] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.playerFace) return;
			Trigger::StaFlags.STA_QTE = 1;
			ctx.playerFace->requestAnimationByName((ctx.Frame <= 255.0f) ? "2e93" : "0000", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			if (ctx.HasValidEnemy() && ctx.TargetEnemy) {
				Hw::cVec4 position;
				((void(__thiscall*)(Behavior*, Hw::cVec4*, Hw::cVec4*))(shared::base + 0x68CE90))(ctx.player, &position, &ctx.TargetEnemy->m_Rot);
				ctx.TargetEnemy->m_Rot.y = Hw::RadAdjust(ctx.player->m_Rot.y + ctx.TargetEnemy->m_Rot.y);
				D3DXMATRIX* playerMatrix = reinterpret_cast<D3DXMATRIX*>(&ctx.player->m_LocalMatrix);
				if (playerMatrix) {
					D3DXVECTOR3 transformedPos;
					D3DXVec3TransformNormal(&transformedPos, reinterpret_cast<const D3DXVECTOR3*>(&position), playerMatrix);
					ctx.TargetEnemy->m_TransPos.x = transformedPos.x + playerMatrix->_41;
					ctx.TargetEnemy->m_TransPos.y = transformedPos.y + playerMatrix->_42;
					ctx.TargetEnemy->m_TransPos.z = transformedPos.z + playerMatrix->_43;
				}
			}
			switch (ctx.player->m_Rno1) {
			case 10:
				if (ctx.player->m_pEntity) ctx.player->requestAnimationByMap(1338, ctx.player->m_pEntity, 0, 0.0f, 1.0f, 0x8100000, -1.0f, 1.0f);
				++ctx.player->m_Rno1;
				break;
			case 11:
				gameFunctionManager.sub_6C80A0(ctx.player, 1.0f, 1.0f);
				if (gameFunctionManager.IsAnimationEnded(ctx.player, 0)) {
					gameFunctionManager.RemoveCollision(ctx.player, 0, 1);
					ctx.player->setIdle(0);
					Trigger::StaFlags.STA_QTE = 0;
				}
				break;
			}
			};
		m_actionHandlers[0x100091] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1341, 0, "");
			};
		m_actionHandlers[0x100092] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1342, 0, "");
			};
		m_actionHandlers[0x100093] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1343, 0, "");
			};
		m_actionHandlers[0x100095] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			gameFunctionManager.CustomActionFromAnimation(ctx.player, 1344, 0, "");
			ctx.PlayerSheath->requestAnimationByMap(5);
			if (ctx.Frame >= 10.0f && ctx.Frame <= 55.0f) gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, 0.28333f, 0.173333f);
			if (ctx.Frame >= 65.0f) {
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, -0.15f, 0.0f);
				((void* (__thiscall*)(Pl0000*, float, float, float, float))(shared::base + 0x77B270))(ctx.player, 0.30000001f, 0.00017453292f, 1.0471976f, 0.0f);
				((cParts * (__thiscall*)(Pl1400*))(shared::base + 0x46F130))(static_cast<Pl1400*>(ctx.player));
			}
			};
		m_actionHandlers[0x100096] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) {
				gameFunctionManager.CustomActionFromAnimation(ctx.player, 1345, 0, "");
				if (ctx.PlayerSheath) ctx.PlayerSheath->requestAnimationByMap(5);
			}
			};
		m_actionHandlers[0x100097] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			gameFunctionManager.CustomThreeStepAnimation(ctx.player, 1346, 1347, 1348);
			if (ctx.animationName == "0082" && ctx.Frame > 91.0f && ctx.Frame < 93.0f)
				gameFunctionManager.Play_Sound((ctx.currentSubPhase == 0x610) ? "em0020_vo_atk_end_1" : "Comn6000_121010", ctx.player, -1, 0);
			};
		m_actionHandlers[0x100099] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1349, 0, "");
			};
		m_actionHandlers[0x100100] = [](ActionContext& ctx) {
			static auto& gameStateManager = GameStateManager::GetInstance();
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			gameFunctionManager.CustomActionFromAnimation(ctx.player, 1350, 0, "");
			if (ctx.player->m_AnimationFrame >= 82.0f && ctx.player->m_AnimationFrame <= 114.0f && ctx.TargetPlayerEnemyDistance <= 2.1f && ctx.IsEnemyIndexEnableQTE) {
				ctx.player->setRno(0x100500, 10, 0, 0);
				if (ctx.HasValidEnemy()) ctx.TargetEnemy->setRno(0x100500, 10, 0, 0);
			}
			};
		m_actionHandlers[0x100500] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.playerFace) return;
			ctx.playerFace->requestAnimationByName((ctx.Frame <= 80.0f) ? "2e93" : "0000", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			if (ctx.animationName == "22c0" && ctx.PlayerSheath) ctx.PlayerSheath->requestAnimationByMap(5);
			if (ctx.HasValidEnemy() && ctx.TargetEnemy) {
				Hw::cVec4 position;
				((void(__thiscall*)(Behavior*, Hw::cVec4*, Hw::cVec4*))(shared::base + 0x68CE90))(ctx.player, &position, &ctx.TargetEnemy->m_Rot);
				ctx.TargetEnemy->m_Rot.y = Hw::RadAdjust(ctx.player->m_Rot.y + ctx.TargetEnemy->m_Rot.y);
				D3DXMATRIX* playerMatrix = reinterpret_cast<D3DXMATRIX*>(&ctx.player->m_LocalMatrix);
				if (playerMatrix) {
					D3DXVECTOR3 transformedPos;
					D3DXVec3TransformNormal(&transformedPos, reinterpret_cast<const D3DXVECTOR3*>(&position), playerMatrix);
					ctx.TargetEnemy->m_TransPos.x = transformedPos.x + playerMatrix->_41;
					ctx.TargetEnemy->m_TransPos.y = transformedPos.y + playerMatrix->_42;
					ctx.TargetEnemy->m_TransPos.z = transformedPos.z + playerMatrix->_43;
				}
			}
			Trigger::StaFlags.STA_QTE = 1;
			switch (ctx.player->m_Rno1) {
			case 10:
				if (ctx.player->m_pEntity) ctx.player->requestAnimationByMap(1339, ctx.player->m_pEntity, 0, 0.0f, 1.0f, 0x8100000, -1.0f, 1.0f);
				++ctx.player->m_Rno1;
				break;
			case 11:
				gameFunctionManager.sub_6C80A0(ctx.player, 1.0f, 1.0f);
				if (gameFunctionManager.IsAnimationEnded(ctx.player, 0)) {
					gameFunctionManager.RemoveCollision(ctx.player, 0, 1);
					ctx.player->setIdle(0);
					Trigger::StaFlags.STA_QTE = 0;
				}
				break;
			}
			};
		m_actionHandlers[0x100200] = [](ActionContext& ctx) {
			if (ctx.playerFace && ctx.Frame >= 0 && ctx.Frame <= 2)
				ctx.playerFace->requestAnimationByName("0280", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			};
		m_actionHandlers[0x100101] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			if (ctx.Frame >= 72.0f && ctx.Frame <= 76.0f && ctx.TargetPlayerEnemyDistance <= 2.1f && ctx.IsEnemyIndexEnableQTE) {
				ctx.player->setRno(0x100501, 10, 0, 0);
				if (ctx.HasValidEnemy()) ctx.TargetEnemy->setRno(0x100501, 10, 0, 0);
			}
			gameFunctionManager.CustomActionFromAnimation(ctx.player, 1351, 0, "");
			if (ctx.Frame > 21.0f && ctx.Frame < 75.0f && ctx.player->m_pSheathEntity && ctx.player->m_pBladeEntity) {
				ctx.player->removeConstraint(4);
				ctx.player->attachObject(4, ctx.player->m_pSheathEntity, ctx.player->m_pBladeEntity, 1808, 0);
			}
			if (ctx.Frame >= 78.0f && ctx.Frame <= 80.0f)
				gameFunctionManager.UpdateSwordConstraints(ctx.player);
			};
		m_actionHandlers[0x100501] = [](ActionContext& ctx) {
			static auto& gameStateManager = GameStateManager::GetInstance();
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.playerFace) return;
			if (ctx.HasValidEnemy() && ctx.TargetEnemy) {
				Hw::cVec4 position;
				((void(__thiscall*)(Behavior*, Hw::cVec4*, Hw::cVec4*))(shared::base + 0x68CE90))(ctx.player, &position, &ctx.TargetEnemy->m_Rot);
				ctx.TargetEnemy->m_Rot.y = Hw::RadAdjust(ctx.player->m_Rot.y + ctx.TargetEnemy->m_Rot.y);
				D3DXMATRIX* playerMatrix = reinterpret_cast<D3DXMATRIX*>(&ctx.player->m_LocalMatrix);
				if (playerMatrix) {
					D3DXVECTOR3 transformedPos;
					D3DXVec3TransformNormal(&transformedPos, reinterpret_cast<const D3DXVECTOR3*>(&position), playerMatrix);
					ctx.TargetEnemy->m_TransPos.x = transformedPos.x + playerMatrix->_41;
					ctx.TargetEnemy->m_TransPos.y = transformedPos.y + playerMatrix->_42;
					ctx.TargetEnemy->m_TransPos.z = transformedPos.z + playerMatrix->_43;
				}
			}
			Trigger::StaFlags.STA_QTE = 1;
			switch (ctx.player->m_Rno1) {
			case 10:
				if (ctx.player->m_pEntity) ctx.player->requestAnimationByMap(1340, ctx.player->m_pEntity, 0, 0.0f, 1.0f, 0x8100000, -1.0f, 1.0f);
				++ctx.player->m_Rno1;
				break;
			case 11:
				gameFunctionManager.sub_6C80A0(ctx.player, 1.0f, 1.0f);
				if (gameFunctionManager.IsAnimationEnded(ctx.player, 0)) {
					ctx.player->setIdle(0);
					Trigger::StaFlags.STA_QTE = 0;
				}
				break;
			}
			gameFunctionManager.UpdateSwordConstraints(ctx.player);
			ctx.playerFace->requestAnimationByName((ctx.Frame <= 88.0f) ? "2e93" : "0000", 0, 0.0f, 1, 0, 0.0f, 1.0f);
			if (gameStateManager.IsMainSamPlayer) {
				if (ctx.currentSubPhase == 0x610) {
					if (ctx.Frame >= 135.0f && ctx.Frame <= 137.0f) gameFunctionManager.Play_Sound("Comn9000_1j1010", ctx.player, -1, 0);
					if (ctx.Frame >= 243.0f && ctx.Frame <= 245.0f) gameFunctionManager.Play_Sound("Comn9000_1k1010", ctx.player, -1, 0);
				}
				else if (ctx.Frame >= 135.0f && ctx.Frame <= 137.0f && gameStateManager.IsStyleChanged)
					gameFunctionManager.Play_Sound("em0020_vo_atk_end_1", ctx.player, -1, 0);
			}
			else if (gameStateManager.IsDLCSamPlayer && ctx.Frame >= 135.0f && ctx.Frame <= 137.0f && gameStateManager.IsStyleChanged)
				gameFunctionManager.Play_Sound("em0020_vo_atk_end_1", ctx.player, -1, 0);
			};
		m_actionHandlers[0x100103] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1352, 0, "");
			};
		m_actionHandlers[0x100104] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player) gameFunctionManager.CustomActionFromAnimation(ctx.player, 1353, 0, "");
			};
		m_actionHandlers[0x100105] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			ctx.PlayerSheath->requestAnimationByMap(5);
			gameFunctionManager.CustomThreeStepAnimation(ctx.player, 1354, 1355, 1356);
			if (ctx.player->m_pAnimationSlot && ctx.player->m_pAnimationSlot->m_pArray[0].field_4 == 1355 && ctx.Frame)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, -0.5f, 0.0f, 0.0f);
			};
		m_actionHandlers[0x100106] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			ctx.PlayerSheath->requestAnimationByMap(5);
			gameFunctionManager.CustomThreeStepAnimation(ctx.player, 1357, 1358, 1359);
			if (ctx.player->m_pAnimationSlot && ctx.player->m_pAnimationSlot->m_pArray[0].field_4 == 1358)
				gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.5f, 0.0f, 0.0f);
			};
		m_actionHandlers[0x100107] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			ctx.PlayerSheath->requestAnimationByMap(5);
			gameFunctionManager.CustomThreeStepAnimation(ctx.player, 1360, 1361, 1362);
			if (ctx.HasValidEnemy() && ctx.player->m_pAnimationSlot) {
				float distance2D = (ctx.player->m_TransPos - ctx.TargetEnemy->m_TransPos).length2D();
				float distance = (distance2D >= 1.5f) ? ctx.TargetPlayerEnemyDistance : 0.0f;
				float distanceMoveY = (ctx.TargetPlayerEnemyDistance >= 10.0f) ? ctx.TargetPlayerEnemyDistance / 50.0f : 0.3333333f;
				float distanceDiv50 = distance / 50.0f;
				int currentAnim = ctx.player->m_pAnimationSlot->m_pArray[0].field_4;
				if (currentAnim == 1361 || currentAnim == 1362) {
					gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, distanceMoveY, distanceDiv50);
					if (distance2D < 10.0f || (gameFunctionManager.IsAnimationEnded(ctx.player, 0) && currentAnim == 1362))
						ctx.player->setRno(0x100108, 0, 0, 0);
				}
			}
			};
		m_actionHandlers[0x100108] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player || !ctx.PlayerSheath) return;
			ctx.PlayerSheath->requestAnimationByMap(5);
			gameFunctionManager.CustomThreeStepAnimation(ctx.player, 129, 130, 131);
			if (ctx.HasValidEnemy() && ctx.player->m_pAnimationSlot) {
				float distance2D = (ctx.player->m_TransPos - ctx.TargetEnemy->m_TransPos).length2D();
				float distance = (distance2D >= 1.5f) ? ctx.TargetPlayerEnemyDistance : 0.0f;
				float distanceZ = std::abs(ctx.player->m_TransPos.z - ctx.TargetEnemy->m_TransPos.z);
				int currentAnim = ctx.player->m_pAnimationSlot->m_pArray[0].field_4;
				if (currentAnim == 129) gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, ctx.TargetPlayerEnemyDistance / 40.0f, distance / 30.0f);
				else if (currentAnim == 130) {
					gameFunctionManager.IncreaseMovementDistance(ctx.player, 0.0f, ctx.TargetPlayerEnemyDistance / 20.0f, distance / 50.0f);
					if (distanceZ <= 1.7333333f) ctx.player->m_Rno1 = 4;
				}
			}
			};
		m_actionHandlers[0x100503] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (!ctx.player) return;
			if (ctx.HasValidEnemy() && ctx.TargetEnemy) {
				Hw::cVec4 position;
				((void(__thiscall*)(Behavior*, Hw::cVec4*, Hw::cVec4*))(shared::base + 0x68CE90))(ctx.player, &position, &ctx.TargetEnemy->m_Rot);
				ctx.TargetEnemy->m_Rot.y = Hw::RadAdjust(ctx.player->m_Rot.y + ctx.TargetEnemy->m_Rot.y);
				D3DXMATRIX* playerMatrix = reinterpret_cast<D3DXMATRIX*>(&ctx.player->m_LocalMatrix);
				if (playerMatrix) {
					D3DXVECTOR3 transformedPos;
					D3DXVec3TransformNormal(&transformedPos, reinterpret_cast<const D3DXVECTOR3*>(&position), playerMatrix);
					ctx.TargetEnemy->m_TransPos.x = transformedPos.x + playerMatrix->_41;
					ctx.TargetEnemy->m_TransPos.y = transformedPos.y + playerMatrix->_42;
					ctx.TargetEnemy->m_TransPos.z = transformedPos.z + playerMatrix->_43;
				}
			}
			Trigger::StaFlags.STA_QTE = 1;
			switch (ctx.player->m_Rno1) {
			case 0:
				if (ctx.player->m_pEntity) ctx.player->requestAnimationByMap(1366, ctx.player->m_pEntity, 0, 0.0f, 1.0f, 0x8100000, -1.0f, 1.0f);
				++ctx.player->m_Rno1;
				break;
			case 1:
				gameFunctionManager.sub_6C80A0(ctx.player, 1.0f, 1.0f);
				if (gameFunctionManager.IsAnimationEnded(ctx.player, 0)) ++ctx.player->m_Rno1;
				break;
			case 2:
				if (ctx.player->m_pEntity) ctx.player->requestAnimationByMap(1367, ctx.player->m_pEntity, 0, 0.0f, 1.0f, 0x8100000, -1.0f, 1.0f);
				++ctx.player->m_Rno1;
				break;
			case 3:
				gameFunctionManager.sub_6C80A0(ctx.player, 1.0f, 1.0f);
				if (gameFunctionManager.IsAnimationEnded(ctx.player, 0)) {
					ctx.player->setIdle(0);
					Trigger::StaFlags.STA_QTE = 0;
				}
				break;
			}
			};
		m_actionHandlers[0x45] = [](ActionContext& ctx) {
			static auto& gameFunctionManager = GameFunctionManager::GetInstance();
			if (ctx.player && (ctx.animationName == "20f4" || ctx.animationName == "2954") && gameFunctionManager.IsAnimationEnded(ctx.player, 0))
				ctx.player->setIdle(0);
			};
		m_actionHandlers[0x100007] = [](ActionContext& ctx) {
			if (ctx.PlayerSheath && ctx.animationName == "Datsu_Blend")
				ctx.PlayerSheath->requestAnimationByMap(5);
			};
		m_actionHandlers[0x10F] = [](ActionContext& ctx) {
			if (!ctx.player) return;
			switch (ctx.player->m_Rno1) {
			case 0: ctx.player->requestAnimationByMap(1400); break;
			case 2: ctx.player->requestAnimationByMap(1401); break;
			case 4: ctx.player->requestAnimationByMap(1402); break;
			}
			};
	}
};

long long PlayerActionHandler::s_lastProcessedCode100010 = 0;
long long PlayerActionHandler::s_lastProcessedCode10000F = 0;
float PlayerActionHandler::s_a1Value = 1.0f;
inline PlayerActionHandler* g_pPlayerActionHandler = &PlayerActionHandler::GetInstance();