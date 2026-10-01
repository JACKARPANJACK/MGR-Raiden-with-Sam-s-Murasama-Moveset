#include "gui.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include <Hw.h>
#include <windows.h>
#include "injector/injector.hpp"



#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#include "../SafeHook/SafeHook.h"

static WNDPROC g_originalWndProc = nullptr;
static bool g_imguiInitialized = false;
static bool g_guiVisible = false;

#include <memory>

static BOOL(WINAPI* oSetCursorPos)(int X, int Y) = nullptr;
static BOOL(WINAPI* oClipCursor)(const RECT* lpRect) = nullptr;
static std::unique_ptr<SafeHook::Hook> g_hookSetCursorPos;
static std::unique_ptr<SafeHook::Hook> g_hookClipCursor;

static BOOL WINAPI Custom_SetCursorPos(int X, int Y)
{
    if (g_guiVisible)
        return TRUE;
    return oSetCursorPos ? oSetCursorPos(X, Y) : SetCursorPos(X, Y);
}

static BOOL WINAPI Custom_ClipCursor(const RECT* lpRect)
{
    if (g_guiVisible)
        return TRUE;
    return oClipCursor ? oClipCursor(lpRect) : ClipCursor(lpRect);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT CALLBACK SamMovesetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_imguiInitialized)
    {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

        if (g_guiVisible)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))
                return 1L;
            if (io.WantCaptureKeyboard && (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))
                return 1L;
        }
    }
    return CallWindowProcA(g_originalWndProc, hwnd, msg, wParam, lParam);
}

void gui::OnResetBefore()
{
    if (g_imguiInitialized)
        ImGui_ImplDX9_InvalidateDeviceObjects();
}

void gui::OnResetAfter()
{
    if (g_imguiInitialized)
        ImGui_ImplDX9_CreateDeviceObjects();
}

static void RenderSamDebug()
{
    static bool was0Down = false;
    const bool is0Down = (GetAsyncKeyState('0') & 0x8000) != 0;

    if (is0Down && !was0Down)
    {
        g_guiVisible = !g_guiVisible;
        if (g_guiVisible)
        {
            ::ClipCursor(nullptr);
            ::ShowCursor(TRUE);
        }
        else
        {
            ::ShowCursor(FALSE);
        }
    }

    was0Down = is0Down;

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = g_guiVisible;

    if (!g_guiVisible)
        return;

    POINT pt;
    if (::GetCursorPos(&pt) && ::ScreenToClient(Hw::OsWindow::m_MainWindow, &pt))
    {
        io.AddMousePosEvent(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }

    gui::DebugView state{};
    gui::GetDebugView(state);

    ImGui::SetNextWindowSize(ImVec2(660.0f, 540.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("MGR: Raiden with Sam's Moveset - Diagnostics (Press '0' to Close)", &g_guiVisible))
    {
        // --- Header Status ---
        if (state.active)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                "=== STATUS: SAM MOVESET ACTIVE (Murasama Combat) ===");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                "=== STATUS: RAIDEN NATIVE COMBAT (Press 'G' or button below) ===");
        }

        ImGui::Spacing();

        // Control Buttons
        if (ImGui::Button(state.active ? "Deactivate Moveset (G)" : "Activate Sam Moveset (G)", ImVec2(220, 28)))
        {
            gui::ToggleMovesetFromGUI();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stream/Reload Resources", ImVec2(180, 28)))
        {
            gui::RequestResourcesFromGUI();
        }

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("X: next ultimate / queue after current ground attack");
        static const char* weaponNames[]={"Raiden sword","Murasama / Sam moveset","Pole-arm","Sai","Pincer blades","Unarmed"};
        int selectedWeapon=gui::PendingWeapon()<0?gui::SelectedWeapon():gui::PendingWeapon();
        if(ImGui::Combo("Weapon",&selectedWeapon,weaponNames,6)) gui::SelectWeapon(selectedWeapon);
        ImGui::TextWrapped("Q / E or D-pad Left / Right: previous / next weapon. D-pad Up: Murasama; Down: Raiden sword. Murasama enables Sam; switching away disables it. G also toggles Sam.");
        if(gui::PendingWeapon()>=0) ImGui::Text("Weapon switch queued...");
        static const char* kunaiNames[]={"Native inventory subweapon","Stun kunai","Explosive kunai","Heat-blade kunai"};
        int selectedKunai=gui::SelectedKunai();
        if(ImGui::Combo("Subweapon",&selectedKunai,kunaiNames,4)) gui::SelectKunai(selectedKunai);
        ImGui::TextWrapped("F7 / F8: previous / next kunai type. Uses grenade ammo and your normal subweapon aim/throw controls. Native inventory restores the previous grenade/RPG selection.");
        ImGui::TextWrapped("Runtime repair: Sam damage table, 20% faster attacks/Ninja Run, nearby-enemy targeting, timed grab impacts and bounded Round Trip hits.");
        ImGui::TextWrapped("F: Sam sweeping finisher on nearby cyborgs below 25% HP. Native executions, Zandatsu and Blade Mode charge use Sam's DLC controller. Regular hits: 12% electric stun; heavy hits: 25%, with a cooldown.");
        ImGui::Text("Next: %s%s", state.nextUltimate, state.ultimateQueued ? " (QUEUED)" : "");
        ImGui::SameLine();
        if (ImGui::Button("Cycle Next Move"))
        {
            gui::CycleNextUltimate();
        }

        ImGui::TextWrapped("Flick + Light: forward rapid slashes, back grab, left sweep, right tackle. Flick + Heavy: forward JCE, back Round Trip, left sonic slash, right leap.");
        ImGui::TextWrapped("Hold direction + Light: forward rapid slashes, back finisher, left sweep, right leap. Hold direction + Heavy: forward charged slash, back stone burst, left sonic slash, right Round Trip.");
        static const char* s_moveNames[] = {
            "Raiden thunder slice (3016 -> 3017)",
            "Raiden lightning storm (3500 -> 3506)",
            "Raiden lightning draw slash (3010 -> 3017)"
        };
        static int s_selectedMove = 0;
        if (ImGui::Combo("Select Move", &s_selectedMove, s_moveNames, 3))
        {
            gui::SelectUltimate(static_cast<unsigned int>(s_selectedMove));
        }
        ImGui::SameLine();
        if (ImGui::Button("Trigger Now"))
        {
            gui::SelectUltimate(static_cast<unsigned int>(s_selectedMove));
            gui::TriggerUltimateNow();
        }

        if (state.roundTripActive)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[ROUND TRIP ACTIVE - Blade autonomously shredding & Raiden unarmed]");
            ImGui::SameLine();
            if (ImGui::Button("Recall Blade Now"))
            {
                gui::RecallRoundTrip();
            }
        }

        ImGui::TextWrapped("Boss moves use directional attacks. X queues a Raiden ultimate. G restores Raiden.");

        // --- Resource & State Info ---
        if (ImGui::CollapsingHeader("Engine & Resource Status", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Columns(2, "resColumns", false);
            ImGui::SetColumnWidth(0, 320.0f);

            ImGui::Text("Resources: %s", state.resourcesLoaded ? "READY (Loaded)" : "STREAMING / WAITING");
            ImGui::Text("pl1400.dat files: %zu", state.pl1400FileCount);
            ImGui::Text("em0020.dat files: %zu", state.em0020FileCount);
            ImGui::Text("Action Handler: %s", state.handlerName);

            ImGui::NextColumn();

            ImGui::Text("Action Rno0: 0x%X  Rno1: 0x%X", state.action0, state.action1);
            ImGui::Text("Playing Anim: %s", state.animationName);
            ImGui::Text("Sam sequences supplied: %u", state.sequenceReplacements);
            ImGui::Text("Boss Ender: %s (%s)", state.bossEnderActive ? "ACTIVE" : "idle", state.bossEnderCode);

            ImGui::Columns(1);
        }

        // --- Replacement Statistics ---
        if (ImGui::CollapsingHeader("Animation Replacement Statistics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const float successRate = state.mapRequests > 0 ?
                (100.0f * static_cast<float>(state.replacements) / static_cast<float>(state.mapRequests)) : 0.0f;

            ImGui::Text("Total Requests: %u   |   Replaced: %u (%.1f%%)   |   Fallbacks: %u",
                state.mapRequests, state.replacements, successRate, state.failures);

            ImGui::Text("Last Request: Map ID %d | Raiden: %s -> Sam: %s (%s)",
                state.lastMapId, state.raidenCode, state.samCode, state.archiveName);

            ImGui::SameLine();
            if (state.lastReplacement)
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[SUCCESS]");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "[FALLBACK]");
        }

        // --- Live Replacement History Table ---
        if (ImGui::CollapsingHeader("Live Replacement History Log (Latest 32 Requests)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("HistoryTable", 6,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                ImVec2(0.0f, 170.0f)))
            {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Map ID", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Raiden", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Sam", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Details / Archive", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < state.historyCount && i < gui::MAX_HISTORY; ++i)
                {
                    const auto& entry = state.history[i];
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%u", entry.frame);

                    ImGui::TableNextColumn();
                    if (entry.animId >= 0)
                        ImGui::Text("%d", entry.animId);
                    else
                        ImGui::TextDisabled("Name");

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", entry.raidenCode);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", entry.samCode);

                    ImGui::TableNextColumn();
                    if (entry.replaced)
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "SUCCESS");
                    else
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "FALLBACK");

                    ImGui::TableNextColumn();
                    ImGui::Text("%s (%s)", entry.details, entry.archiveName);
                }
                ImGui::EndTable();
            }
        }

        // --- Direct Animation Tester ---
        if (ImGui::CollapsingHeader("Direct Animation Tester"))
        {
            static char testCode[16] = "2000";
            static char statusText[96] = "Ready to test";

            ImGui::InputText("Sam Anim Code", testCode, sizeof(testCode));
            ImGui::SameLine();
            if (ImGui::Button("Play on Player"))
            {
                gui::PlayTestAnimation(testCode, statusText, sizeof(statusText));
            }
            ImGui::Text("Test Status: %s", statusText);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Debug log written to scripts\\RaidenMoveset-debug.log");
    }
    ImGui::End();
}

void gui::OnEndScene()
{
    if (!g_imguiInitialized)
    {
        HWND window = Hw::OsWindow::m_MainWindow;
        IDirect3DDevice9* device = Hw::GraphicDevice::m_pDevice;
        if (!window || !device)
            return;
        g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(SamMovesetWndProc)));

        g_hookSetCursorPos = std::make_unique<SafeHook::Hook>(
            (void*)&SetCursorPos,
            (void*)Custom_SetCursorPos,
            true,
            (void**)&oSetCursorPos
        );
        g_hookClipCursor = std::make_unique<SafeHook::Hook>(
            (void*)&ClipCursor,
            (void*)Custom_ClipCursor,
            true,
            (void**)&oClipCursor
        );

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX9_Init(device);
        g_imguiInitialized = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    RenderSamDebug();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void gui::OnMainCleanup()
{
    if (!g_imguiInitialized)
        return;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (Hw::OsWindow::m_MainWindow && g_originalWndProc)
        SetWindowLongPtr(Hw::OsWindow::m_MainWindow, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(g_originalWndProc));
    g_originalWndProc = nullptr;
    g_imguiInitialized = false;
}
