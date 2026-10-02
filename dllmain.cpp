#include <cScene.h>
#include <Pl0000.h>
#include <shared.h>
#include "injector/injector.hpp"
#include <windows.h>
#include "../SafeHook/SafeHook.h"

#include "SamMovesetManager.h"
#include "WeaponSwitcher.h"
#include "KunaiSubweapon.h"
#include "NativeSmgMenu.h"
#include "WeaponSlowMotionFix.h"
#include "SamDlcRouting.h"
#include "SamCombatRouting.h"
#include "gui.h"
#include <Events.h>
#include "SamConfig.h"
#include <intrin.h>
#include "SamEffectPolicy.h"

void gui::GetDebugView(gui::DebugView& out)
{
    SamMovesetManager::DebugSnapshot state{};
    SamMovesetManager::Instance().CopyDebugSnapshot(state);
    out.requested = state.requested;
    out.active = state.active;
    out.resourcesLoaded = state.resourcesLoaded;
    out.lastReplacement = state.lastReplacement;
    out.bossEnderActive = state.bossEnderActive;
    out.roundTripActive = state.roundTripActive;
    out.ultimateQueued = SamMovesetManager::Instance().UltimateQueued();
    std::snprintf(out.nextUltimate, sizeof(out.nextUltimate), "%s", SamMovesetManager::Instance().NextUltimateName());
    out.mapRequests = state.mapRequests;
    out.replacements = state.replacements;
    out.sequenceReplacements = state.sequenceReplacements;
    out.failures = state.failures;
    out.action0 = state.action0;
    out.action1 = state.action1;
    out.lastMapId = state.lastMapId;
    out.chargeFrames = state.chargeFrames;
    out.chargeTier = state.chargeTier;
    std::memcpy(out.handlerName, state.handlerName, sizeof(out.handlerName));
    std::memcpy(out.raidenCode, state.raidenCode, sizeof(out.raidenCode));
    std::memcpy(out.samCode, state.samCode, sizeof(out.samCode));
    std::memcpy(out.archiveName, state.archiveName, sizeof(out.archiveName));
    std::memcpy(out.animationName, state.animationName, sizeof(out.animationName));
    std::memcpy(out.bossEnderCode, state.bossEnderCode, sizeof(out.bossEnderCode));
    std::memcpy(out.history, state.history, sizeof(out.history));
    out.historyCount = state.historyCount;
    out.pl1400FileCount = state.pl1400FileCount;
    out.em0020FileCount = state.em0020FileCount;
}

void gui::ToggleMovesetFromGUI()
{
    if (g_Scene.m_pPlayer && reinterpret_cast<uintptr_t>(g_Scene.m_pPlayer) >= 0x10000)
        SamMovesetManager::Instance().Toggle(g_Scene.m_pPlayer);
}

int gui::SelectedWeapon() { return WeaponSwitcher::Get().Selected(); }
int gui::PendingWeapon() { return WeaponSwitcher::Get().Pending(); }
void gui::SelectWeapon(int index) { WeaponSwitcher::Get().Select(index); }
int gui::SelectedKunai() { return KunaiSubweapon::Get().Pending()<0 ? KunaiSubweapon::Get().Selected() : KunaiSubweapon::Get().Pending(); }
int gui::EquippedSecondary() { return KunaiSubweapon::Get().Selected(); }
int gui::PendingSecondary() { return KunaiSubweapon::Get().Pending(); }
void gui::GetKunaiAimView(gui::KunaiAimView& out) { KunaiSubweapon::Get().AimView(out); }
void gui::GetSmgView(gui::SmgView& out) { KunaiSubweapon::Get().SmgView(out); }
void gui::SelectKunai(int index) { KunaiSubweapon::Get().Select(index); }
static Behavior* __cdecl ReleaseKunai(Entity* owner, void* descriptor)
{ return KunaiSubweapon::Get().Release(owner,descriptor); }

void gui::RequestResourcesFromGUI()
{
    SamResourceManager::Instance().RequestAllSamResources();
}

void gui::CycleNextUltimate()
{
    SamMovesetManager::Instance().CycleUltimate();
}

void gui::SelectUltimate(unsigned int index)
{
    SamMovesetManager::Instance().SetUltimateIndex(index);
}

void gui::TriggerUltimateNow()
{
    SamMovesetManager::Instance().TriggerUltimateNow();
}

void gui::RecallRoundTrip()
{
    if (g_Scene.m_pPlayer && reinterpret_cast<uintptr_t>(g_Scene.m_pPlayer) >= 0x10000)
        SamMovesetManager::Instance().EndRoundTrip(g_Scene.m_pPlayer);
}

bool gui::PlayTestAnimation(const char* code, char* outDetails, size_t outDetailsCap)
{
    Pl0000* player = g_Scene.m_pPlayer;
    if (!player || reinterpret_cast<uintptr_t>(player) < 0x10000)
    {
        if (outDetails && outDetailsCap > 0)
            std::snprintf(outDetails, outDetailsCap, "Player not available");
        return false;
    }

    const char* resolved = SamMovesetManager::ResolveSamCode(code);
    const auto clip = SamResourceManager::Instance().GetClip(
        resolved ? resolved : code,
        SamArchiveLookup::Source::Playable, true, false);

    if (clip.motion)
    {
        player->setDirectAnimation(clip.motion, clip.sequence, 0,
            0.05f, 1.0f, 0x8000000, 0.0f, 1.0f);

        if (player->m_pSheathEntity && player->m_pSheathEntity->m_pBehavior)
        {
            void* sheathMot = SamResourceManager::Instance().GetSheathMotion(
                resolved ? resolved : code);
            if (sheathMot)
                player->m_pSheathEntity->m_pBehavior->setDirectAnimation(sheathMot, nullptr, 0,
                    0.05f, 1.0f, 0x8000000, 0.0f, 1.0f);
        }

        if (outDetails && outDetailsCap > 0)
            std::snprintf(outDetails, outDetailsCap, "Playing clip %s (%s)",
                resolved ? resolved : code,
                clip.source == SamArchiveLookup::Source::Playable ? "pl1400" : "em0020");
        return true;
    }

    if (outDetails && outDetailsCap > 0)
        std::snprintf(outDetails, outDetailsCap, "Clip %s NOT found in archives", code ? code : "-");
    return false;
}

static void(__cdecl* g_origTickGame)() = nullptr;
using PlayerFactory = Pl0000*(__cdecl*)(void*);
static PlayerFactory oPlayerFactory = nullptr;
static Pl0000* __cdecl CreateExtendedPlayer(void* heap)
{
    Pl0000* player = oPlayerFactory(heap);
    SamNativeRuntime::Get().Created(player);
    return player;
}
using PlayerShutdown = void(__thiscall*)(Pl0000*);
static PlayerShutdown oPlayerShutdown = nullptr;
static void __fastcall ShutdownExtendedPlayer(Pl0000* player, void*)
{
    WeaponSwitcher::Get().Forget(player);
    KunaiSubweapon::Get().Forget(player);
    SamNativeRuntime::Get().BeforeShutdown(player);
    SamMovesetManager::Instance().ForgetPlayer(player);
    oPlayerShutdown(player);
    SamNativeRuntime::Get().Destroyed(player);
    if (!g_Scene.m_pPlayer || g_Scene.m_pPlayer == player)
    {
        KunaiSubweapon::Get().SceneReleased();
        SamResourceManager::Instance().SceneReleased();
    }
}

typedef void(__thiscall* Pl0000_HandleActions_t)(Pl0000* pThis);
static Pl0000_HandleActions_t oPl0000_HandleActions = nullptr;

typedef int(__thiscall* Behavior_RequestAnimationByMap_t)(Behavior* pThis, int animationId);
static Behavior_RequestAnimationByMap_t oBehavior_RequestAnimationByMap = nullptr;

typedef int(__thiscall* Behavior_RequestAnimationByName_t)(Behavior* pThis, const char* anim, int node,
    float interpolation, float weight, unsigned int flags, float startFrame, float playbackSpeed);
static Behavior_RequestAnimationByName_t oBehavior_RequestAnimationByName = nullptr;

// Motion lookup uses hashed archive entries, not cFmerge::getFileNameData.
// Sequence lookup is a separate Behavior call using a temporary archive copy.
using Animation_FindMotion_t = void* (__thiscall*)(Animation*, const char*);
static Animation_FindMotion_t oAnimation_FindMotion = nullptr;
using Behavior_GetSequenceFile_t = void* (__thiscall*)(Behavior*, const char*);
static Behavior_GetSequenceFile_t oBehavior_GetSequenceFile = nullptr;
using Animation_RequestSequence_t = void(__thiscall*)(Animation*, const char*, int);
static Animation_RequestSequence_t oAnimation_RequestSequence = nullptr;

struct PlayerAnimationRequest : SamCombatRouting::RequestClip
{
    Behavior* actor = nullptr;
    int mapId = -1;
    bool motionSupplied = false;
};
static thread_local PlayerAnimationRequest* s_animationRequest = nullptr;
static thread_local int s_animationMapId = -1;

static bool IsMovesetPlayer(Behavior* behavior)
{
    return behavior && SamMovesetManager::Instance().IsEnabled() &&
        !SamMovesetManager::Instance().IsSubweaponActive() &&
        !SamMovesetManager::StoryEvent() && SamNativeRuntime::Get().Active(g_Scene.m_pPlayer) &&
        behavior == reinterpret_cast<Behavior*>(g_Scene.m_pPlayer);
}

using CreatePlayerEffect = void(__thiscall*)(Behavior*, int, cEspControler*);
static CreatePlayerEffect oCreatePlayerEffect = nullptr;
static void __fastcall Custom_CreatePlayerEffect(Behavior* actor, void*, int id, cEspControler* controller)
{
    const uintptr_t caller = reinterpret_cast<uintptr_t>(_ReturnAddress()) - shared::base;
    const int mapped = SamEffectPolicy::Remap(id, caller, IsMovesetPlayer(actor));
    if (mapped != id && SamNativeRuntime::Get().CreatePlayerEffect(
        static_cast<Pl0000*>(actor),mapped,controller)) return;
    if (oCreatePlayerEffect) oCreatePlayerEffect(actor, id, controller);
}

static void __fastcall Custom_Animation_RequestSequence(Animation* animation, void*, const char* name, int node)
{
    auto* request = s_animationRequest;
    auto* player = g_Scene.m_pPlayer;
    if (request && request->motionSupplied && request->clip.sequence &&
        IsMovesetPlayer(request->actor) && player && player->m_pEntity &&
        player->m_pEntity->m_pAnimation == animation)
    {
        using AttachSequence = int(__thiscall*)(void*, void*, const char*, int);
        const int result = reinterpret_cast<AttachSequence>(shared::base + 0xA3FA90)(
            reinterpret_cast<char*>(animation) + 0xF4, request->clip.sequence, name, node);
        if (result != -1) SamMovesetManager::Instance().RecordSequenceReplacement(request->samCode);
        return;
    }
    if (oAnimation_RequestSequence) oAnimation_RequestSequence(animation, name, node);
}

static void* __fastcall Custom_Animation_FindMotion(Animation* animation, void*, const char* name)
{
    auto* request = s_animationRequest;
    Pl0000* player = g_Scene.m_pPlayer;
    if (request && IsMovesetPlayer(request->actor) && player && player->m_pEntity &&
        animation == player->m_pEntity->m_pAnimation && request->clip.sequence &&
        (request->clip.motion || SamDlcRouting::SharedMotion(request->samCode)))
    {
        void* motion=request->clip.motion;
        if(!motion && oAnimation_FindMotion) motion=oAnimation_FindMotion(animation,name);
        if(!motion) return nullptr;
        SamMovesetManager::Instance().RecordAnimationMapRequest(request->mapId,
            request->code, request->samCode,
            request->clip.source == SamArchiveLookup::Source::Playable ? "pl1400" : "em0020",
            true, request->clip.motion?"Native Sam motion and sequence":"DLC sequence with native shared execution motion");
        request->motionSupplied = true;
        return motion;
    }
    return oAnimation_FindMotion ? oAnimation_FindMotion(animation, name) : nullptr;
}

static void* __fastcall Custom_Behavior_GetSequenceFile(Behavior* actor, void*, const char* name)
{
    auto* request = s_animationRequest;
    // Use the exact pair selected by the motion lookup for this request.
    // Do not resolve again or mix the two Sam archives.
    if (request && request->motionSupplied && actor == request->actor && IsMovesetPlayer(actor))
        if (void* sequence = request->clip.sequence)
        {
            SamMovesetManager::Instance().RecordSequenceReplacement(request->code);
            return sequence;
        }
    return oBehavior_GetSequenceFile ? oBehavior_GetSequenceFile(actor, name) : nullptr;
}

static int __fastcall Custom_Behavior_RequestAnimationByMap(Behavior* pThis, void*, int animationId)
{
    const bool active = IsMovesetPlayer(pThis);
    const int previousMapId = s_animationMapId;
    s_animationMapId = active ? animationId : -1;
    int result = 0;
    __try
    {
        if (oBehavior_RequestAnimationByMap)
            result = oBehavior_RequestAnimationByMap(pThis, animationId);
    }
    __finally
    {
        s_animationMapId = previousMapId;
    }
    return result;
}

static int __fastcall Custom_Behavior_RequestAnimationByName(Behavior* pThis, void*,
    const char* anim, int node, float interpolation, float weight, unsigned int flags,
    float startFrame, float playbackSpeed)
{
    const bool active = IsMovesetPlayer(pThis);
    PlayerAnimationRequest request{};
    request.actor = active ? pThis : nullptr;
    request.mapId = s_animationMapId;
    const bool nativeSam = active && SamNativeRuntime::Get().Active(g_Scene.m_pPlayer);
    if (active && anim && (std::strstr(anim, "3500") || std::strstr(anim, "3501")))
    {
        SamMovesetManager::Instance().TriggerAddon(7);
    }
    char bladeCode[5]{};
    // Native Sam state nodes also request 4xxx/8xxx/9xxx clips for taunts,
    // finishers and Datsu. Resolve their DLC pair without combat remapping.
    const bool samNativeClip=SamDlcRouting::Code(bladeCode,anim,nativeSam,
        nativeSam?g_Scene.m_pPlayer->m_Rno0:0,nativeSam&&g_Scene.m_pPlayer->isBladeModeActive());
    if (active && (SamCombatRouting::CodeFromRequest(anim) ||
        (nativeSam && SamCombatRouting::IsNativeAirTransition(anim)) ||
        samNativeClip))
    {
        bool paired = false;
        if (nativeSam)
        {
            const char* code = SamCombatRouting::CodeFromRequest(anim);
            if (!code) code = bladeCode[0] ? bladeCode : anim;
            std::memcpy(request.code, code, 4);
            std::memcpy(request.samCode, code, 4);
            request.clip = SamResourceManager::Instance().GetClip(request.samCode,
                SamArchiveLookup::Source::Playable, true, true);
            if(!request.clip.sequence && SamDlcRouting::SharedMotion(request.samCode))
            {
                char sharedSequence[64]{};
                std::snprintf(sharedSequence,sizeof(sharedSequence),"pl1400_%s_2_seq.bxm",request.samCode);
                request.clip.sequence=SamResourceManager::Instance().GetRawFile(sharedSequence);
                request.clip.source=SamArchiveLookup::Source::Playable;
            }
            if (request.clip.source == SamArchiveLookup::Source::Boss && request.clip.sequence)
                request.clip.sequence = SamMovesetManager::Instance().BossSequence(request.samCode,request.clip.sequence);
            paired = request.clip.sequence && (request.clip.motion || SamDlcRouting::SharedMotion(request.samCode));
        }
        else paired = request.Select(anim, [](const char* samCode) {
            return SamResourceManager::Instance().GetClip(samCode,
                SamArchiveLookup::Source::Playable, true, true);
            });
        if (!paired)
            SamMovesetManager::Instance().RecordAnimationMapRequest(request.mapId,
                request.code, request.samCode, "pl1400/em0020", false, "Sam pair missing; native fallback");
        else if (request.clip.motion && request.clip.sequence)
        {
            Pl0000* player = g_Scene.m_pPlayer;
            if (player) SheathController::Instance().AnimateSheathDirect(player, request.samCode);
        }
    }
    PlayerAnimationRequest* previousRequest = s_animationRequest;
    s_animationRequest = active ? &request : nullptr;
    if (active)
        SamMovesetManager::Instance().TraceRequest(anim);
    int result = 0;
    __try
    {
        if (oBehavior_RequestAnimationByName)
            result = oBehavior_RequestAnimationByName(pThis, anim, node, interpolation,
                weight, flags, startFrame, active && SamCombatRouting::IsCombatCode(anim) &&
                    !g_Scene.m_pPlayer->isBladeModeActive() ? playbackSpeed*SamBalancePolicy::AttackSpeed : playbackSpeed);
    }
    __finally
    {
        s_animationRequest = previousRequest;
    }
    return result;
}

// ============================================================================
// Pl0000_HandleActions hook (Raiden's handler at 0x805000)
// ============================================================================
static void __fastcall Custom_Pl0000_HandleActions(Pl0000* pThis, void* edx)
{
    if (pThis == g_Scene.m_pPlayer)
        SamMovesetManager::Instance().RecordActionHandler("raiden");

    if (oPl0000_HandleActions)
        oPl0000_HandleActions(pThis);

    if (pThis == g_Scene.m_pPlayer && SamMovesetManager::Instance().IsBossEnderActive())
    {
        if (pThis->m_Rno0 != SamUltimatePolicy::Action && SamUltimatePolicy::Attack(pThis->m_Rno0))
        {
            pThis->setRno(SamUltimatePolicy::Action, 0, 0, 0);
        }
    }
}

// ============================================================================
// TickGame hook - runs toggle + maintenance every frame
// ============================================================================
static void __cdecl CustomTickGame()
{
    __try
    {
        SamResourceManager::Instance().Update();

        Pl0000* player = g_Scene.m_pPlayer;
        if (player && reinterpret_cast<uintptr_t>(player) >= 0x10000 &&
            player->m_pEntity && reinterpret_cast<uintptr_t>(player->m_pEntity) >= 0x10000)
        {
            SamMovesetManager::Instance().UpdateSceneSafety(player);
            WeaponSwitcher::Get().Tick(player);
            KunaiSubweapon::Get().Tick(player);
            SamMovesetManager::Instance().OnTick(player);
            WeaponSwitcher::Get().ReserveUltimate(player);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    __try
    {
        if (g_origTickGame) g_origTickGame();
    }
    __finally { KunaiSubweapon::Get().RestoreInputs(); WeaponSwitcher::Get().RestoreInputs(); }
    __try
    {
        auto* player = g_Scene.m_pPlayer;
        if (player && player->m_pEntity) SamMovesetManager::Instance().PostTick(player);
        if (player && player->m_pEntity) KunaiSubweapon::Get().PostTick(player);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

}

// ============================================================================
// Hook installation
// ============================================================================
static void InitHooks()
{
    SamConfig::LoadConfig();
    WeaponSlowMotionFix::Install();
    static SafeHook::Hook knifeDefinition((void*)(shared::base+0x54DFD0),
        (void*)NativeKnifeInventory::LookupDefinition,true,(void**)&NativeKnifeInventory::originalDefinition);
    static SafeHook::Hook knifeAlias((void*)(shared::base+0x77F840),
        (void*)NativeKnifeInventory::EquippedAlias,true,(void**)&NativeKnifeInventory::originalEquippedAlias);
      static SafeHook::Hook knifeDefinitionById((void*)(shared::base+0x54DF20),
        (void*)NativeKnifeInventory::LookupDefinitionById,true,(void**)&NativeKnifeInventory::originalDefinitionById);
    NativeSmgMenu::Install();
    // Native 7A4410 grenade throw release, after its ammo consumption and aim.
    if(KunaiSubweapon::Get().Install()) injector::MakeCALL(shared::base+0x7A4883,ReleaseKunai);
    // Hard reset the opt-in state
    g_GameStateManager.IsMainSamPlayer = false;
    g_GameStateManager.InstantCharge = false;
    SamMovesetManager::Instance().ResetForNativeDlc();
    if (SamNativeRuntime::Get().PrepareAllocation())
    {
        static SafeHook::Hook factory((void*)(shared::base + 0x6C3340),
            (void*)CreateExtendedPlayer, true, (void**)&oPlayerFactory);
        static SafeHook::Hook shutdown((void*)(shared::base + 0x7BCEE0),
            (void*)ShutdownExtendedPlayer, true, (void**)&oPlayerShutdown);
    }
    Events::OnDeviceReset.before += gui::OnResetBefore;
    Events::OnDeviceReset.after += gui::OnResetAfter;
    Events::OnEndScene += gui::OnEndScene;
    // No global dispatch patches were installed, so there is nothing to
    // restore here. Writing assumed defaults would overwrite other plugins.

    // Install TickGame hook
    g_origTickGame = reinterpret_cast<void(__cdecl*)()>(
        injector::MakeCALL(shared::base + 0x64D411, CustomTickGame).get<void>()
        );

    // Install Pl0000_HandleActions hook (Raiden's handler)
    static SafeHook::Hook g_hookPl0000_HandleActions(
        (void*)(shared::base + 0x805000),
        (void*)Custom_Pl0000_HandleActions,
        true,
        (void**)&oPl0000_HandleActions
    );

    static SafeHook::Hook g_hookBehavior_RequestAnimationByMap(
        (void*)(shared::base + 0x6A3F60),
        (void*)Custom_Behavior_RequestAnimationByMap,
        true,
        (void**)&oBehavior_RequestAnimationByMap
    );

    static SafeHook::Hook g_hookBehavior_RequestAnimationByName(
        (void*)(shared::base + 0x69E290),
        (void*)Custom_Behavior_RequestAnimationByName,
        true,
        (void**)&oBehavior_RequestAnimationByName
    );

    static SafeHook::Hook g_hookAnimation_FindMotion(
        (void*)(shared::base + 0xA355E0),
        (void*)Custom_Animation_FindMotion,
        true,
        (void**)&oAnimation_FindMotion
    );
    static SafeHook::Hook sequence((void*)(shared::base + 0xA3FC50),
        (void*)Custom_Animation_RequestSequence, true, (void**)&oAnimation_RequestSequence);
    static SafeHook::Hook g_hookBehavior_GetSequenceFile(
        (void*)(shared::base + 0x696360),
        (void*)Custom_Behavior_GetSequenceFile,
        true,
        (void**)&oBehavior_GetSequenceFile
    );
    static SafeHook::Hook effects((void*)(shared::base + 0x7C3470),
        (void*)Custom_CreatePlayerEffect, true, (void**)&oCreatePlayerEffect);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        InitHooks();
    }
    return TRUE;
}
