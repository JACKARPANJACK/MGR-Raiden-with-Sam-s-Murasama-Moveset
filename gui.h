#pragma once
#include <cstddef>

namespace gui
{
    struct KunaiAimView { bool active=false; unsigned frames=0,targets=0,ammo=0,recovery=0; int variant=0; };
    void GetKunaiAimView(KunaiAimView& out);
    bool IsMenuVisible();
    static constexpr size_t MAX_HISTORY = 32;

    struct HistoryEntry
    {
        unsigned int frame;
        int animId;
        char raidenCode[16];
        char samCode[16];
        char archiveName[16];
        bool replaced;
        char details[48];
    };

    struct DebugView
    {
        bool requested, active, resourcesLoaded, lastReplacement, bossEnderActive, roundTripActive;
        bool ultimateQueued;
        char nextUltimate[64];
        unsigned int mapRequests, replacements, sequenceReplacements, failures, action0, action1, lastMapId, chargeFrames, chargeTier;
        char handlerName[32], raidenCode[16], samCode[16], archiveName[16], animationName[96], bossEnderCode[16];
        HistoryEntry history[MAX_HISTORY];
        size_t historyCount;
        size_t pl1400FileCount;
        size_t em0020FileCount;
    };

    void GetDebugView(DebugView& out);
    void ToggleMovesetFromGUI();
    void RequestResourcesFromGUI();
    void CycleNextUltimate();
    void SelectUltimate(unsigned int index);
    void TriggerUltimateNow();
    void RecallRoundTrip();
    int SelectedWeapon();
    int PendingWeapon();
    void SelectWeapon(int index);
    int SelectedKunai();
    void SelectKunai(int index);
    bool PlayTestAnimation(const char* code, char* outDetails, size_t outDetailsCap);
    void OnEndScene();
    void OnResetBefore();
    void OnResetAfter();
    void OnMainCleanup();
}
