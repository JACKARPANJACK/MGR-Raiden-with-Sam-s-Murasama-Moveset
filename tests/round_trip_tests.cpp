#include "../SamRoundTripPolicy.h"
#include "../SamUltimatePolicy.h"
#include "../SamBossSequence.h"
#include "../SamTogglePolicy.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Vec4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

// Standalone simulation harness for Round Trip state transitions and trajectory math
struct RoundTripSim
{
    bool active = false;
    bool unarmed = false;
    int handAttachState = 1; // 1 = hand (1792), 0 = detached / sheath
    int swordHidden = 0;
    unsigned int playerRno0 = 0x100000;
    int constraint4Bone = 1792;
    bool constraint4Attached = true;

    unsigned ticks = 0;
    unsigned hitTicks = 0;
    unsigned damageDealt = 0;
    unsigned hitCount = 0;

    Vec4 bladePos{0, 0, 0, 1};
    Vec4 playerPos{0, 0, 0, 1};
    Vec4 targetPos{0, 0, 10.0f, 1};
    float playerYaw = 0.0f; // facing +Z

    void Throw()
    {
        active = true;
        ticks = 0;
        hitTicks = 0;
        damageDealt = 0;
        hitCount = 0;

        // Detach constraint 4
        constraint4Attached = false;
        constraint4Bone = 0;
        handAttachState = 0;
        swordHidden = 1;
        unarmed = true;

        // Blade spawn position: player + 1.5 units forward, 1.2 units up
        bladePos.x = playerPos.x + std::sin(playerYaw) * 1.5f;
        bladePos.y = playerPos.y + 1.2f;
        bladePos.z = playerPos.z + std::cos(playerYaw) * 1.5f;

        // Player transitions to unarmed idle after throw animation finishes
        playerRno0 = 0x100008;
    }

    void Tick(bool earlyRecallInput = false)
    {
        if (!active) return;
        ++ticks;

        // Early recall after 25 ticks
        if (ticks > 25 && ticks < 130 && earlyRecallInput)
        {
            ticks = 130;
        }

        // Phase 1: Attack target
        if (ticks < 130)
        {
            float dx = targetPos.x - bladePos.x;
            float dy = targetPos.y - bladePos.y;
            float dz = targetPos.z - bladePos.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist > 0.35f)
            {
                SamRoundTripPolicy::Step(bladePos.x, bladePos.y, bladePos.z, targetPos.x, targetPos.y, targetPos.z, 0.55f);
            }

            if (++hitTicks >= 8)
            {
                hitTicks = 0;
                if (SamRoundTripPolicy::CanHit(targetPos.x-bladePos.x, targetPos.y-bladePos.y, targetPos.z-bladePos.z, true))
                { damageDealt += 45; ++hitCount; }
            }
        }
        // Phase 2: Return to player
        else
        {
            Vec4 returnPos = playerPos;
            returnPos.y += 1.0f;
            float dx = returnPos.x - bladePos.x;
            float dy = returnPos.y - bladePos.y;
            float dz = returnPos.z - bladePos.z;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist > 0.75f && ticks < 260)
            {
                SamRoundTripPolicy::Step(bladePos.x, bladePos.y, bladePos.z, returnPos.x, returnPos.y, returnPos.z, 0.70f);

                // Return path damage if near target
                if (++hitTicks >= 8)
                {
                    hitTicks = 0;
                    float tdx = targetPos.x - bladePos.x;
                    float tdy = targetPos.y - bladePos.y;
                    float tdz = targetPos.z - bladePos.z;
                    if (SamRoundTripPolicy::CanHit(tdx, tdy, tdz, true))
                    {
                        damageDealt += 35;
                        ++hitCount;
                    }
                }
            }
            else
            {
                Catch();
            }
        }
    }

    void Catch()
    {
        active = false;
        ticks = 0;
        hitTicks = 0;
        unarmed = false;
        swordHidden = 0;
        handAttachState = 1;
        constraint4Attached = true;
        constraint4Bone = 1792;

        // Restore armed stance
        if (playerRno0 == 0x100008 || playerRno0 == 0x10000E)
        {
            playerRno0 = 0x100000;
        }
    }
};

int main()
{
    // 1. Verify Move Policy Configuration
    assert(SamUltimatePolicy::Count == 14);
    const auto& rtMove = SamUltimatePolicy::Moves[12];
    assert(std::string(rtMove.name) == "Boss Murasama round trip throw");
    assert(std::string(rtMove.windup) == "3200");
    assert(std::string(rtMove.release) == "3210");

    // Verify motion and sequence assets exist on disk
    for (const char* code : {rtMove.windup, rtMove.release})
    {
        std::string motPath = std::string("local_assets/data000/em/em0020.dat.unpacked/em0020_") + code + ".mot";
        std::string seqPath = std::string("local_assets/data000/em/em0020.dat.unpacked/em0020_") + code + "_0_seq.bxm";
        assert(std::ifstream(motPath, std::ios::binary).good());
        assert(std::ifstream(seqPath, std::ios::binary).good());
    }

    // 2. Verify GameStateManager flag presence and code integration in headers
    {
        std::ifstream gvFile("1-GlobleVarious.h", std::ios::binary);
        assert(gvFile.good());
        std::string gvContent((std::istreambuf_iterator<char>(gvFile)), {});
        assert(gvContent.find("bool IsRoundTripActive = false;") != std::string::npos);
        assert(gvContent.find("IsRoundTripActive = false;") != std::string::npos);

        std::ifstream gfmFile("1-GameFunctionManager.h", std::ios::binary);
        assert(gfmFile.good());
        std::string gfmContent((std::istreambuf_iterator<char>(gfmFile)), {});
        assert(gfmContent.find("g_GameStateManager.IsRoundTripActive") != std::string::npos);

        std::ifstream smmFile("SamMovesetManager.h", std::ios::binary);
        assert(smmFile.good());
        std::string smmContent((std::istreambuf_iterator<char>(smmFile)), {});
        assert(smmContent.find("g_GameStateManager.IsRoundTripActive = true;") != std::string::npos);
        assert(smmContent.find("g_GameStateManager.IsRoundTripActive = false;") != std::string::npos);
        assert(smmContent.find("m_roundTripActive = true;") != std::string::npos);
        assert(smmContent.find("m_roundTripActive = false;") != std::string::npos);
        assert(smmContent.find("attachObject(4, player->m_pEntity, sword, 1792, 0)") != std::string::npos);
        assert(smmContent.find("setConstraintsBone(4, 1792, 0)") != std::string::npos);
        assert(smmContent.find("0x811BE0") != std::string::npos);
    }

    // 3. Verify Full Flight Cycle Simulation (Natural 130-tick return)
    {
        RoundTripSim sim;
        assert(!sim.active);
        assert(!sim.unarmed);
        assert(sim.constraint4Attached);
        assert(sim.handAttachState == 1);

        sim.Throw();
        assert(sim.active);
        assert(sim.unarmed);
        assert(!sim.constraint4Attached);
        assert(sim.handAttachState == 0);
        assert(sim.swordHidden == 1);
        assert(sim.playerRno0 == 0x100008);

        // Blade spawn position check (facing +Z, yaw=0)
        assert(std::abs(sim.bladePos.x - 0.0f) < 0.001f);
        assert(std::abs(sim.bladePos.y - 1.2f) < 0.001f);
        assert(std::abs(sim.bladePos.z - 1.5f) < 0.001f);

        // Advance 128 ticks (Phase 1 homing & attack)
        for (unsigned i = 0; i < 128; ++i)
        {
            sim.Tick();
            assert(sim.active);
            assert(sim.unarmed);
        }
        // At 128 ticks, blade has reached target and dealt multi-hit damage
        assert(sim.hitCount >= 12); // ~128/8 = 16 intervals
        assert(sim.damageDealt >= 540);

        // Distance to target should be close
        float distToTarget = std::sqrt(
            std::pow(sim.targetPos.x - sim.bladePos.x, 2.0f) +
            std::pow(sim.targetPos.y - sim.bladePos.y, 2.0f) +
            std::pow(sim.targetPos.z - sim.bladePos.z, 2.0f));
        assert(distToTarget <= 0.40f);

        // Now advance into Phase 2 (>= 130 ticks) until blade returns and catches
        unsigned returnTicks = 0;
        while (sim.active && returnTicks < 150)
        {
            sim.Tick();
            ++returnTicks;
        }

        // Blade should have returned and caught
        assert(!sim.active);
        assert(!sim.unarmed);
        assert(sim.constraint4Attached);
        assert(sim.constraint4Bone == 1792);
        assert(sim.handAttachState == 1);
        assert(sim.swordHidden == 0);
        assert(sim.playerRno0 == 0x100000); // restored armed stance
    }

    // 4. Verify Early Recall Cycle (Player inputs recall at tick 30)
    {
        RoundTripSim sim;
        sim.Throw();
        assert(sim.active);

        // Advance 30 ticks without recall
        for (unsigned i = 0; i < 30; ++i)
            sim.Tick(false);
        assert(sim.ticks == 30);
        assert(sim.active);

        // Trigger early recall at tick 30
        sim.Tick(true);
        // Ticks should fast-forward to Phase 2 (>= 130)
        assert(sim.ticks >= 130);

        // Run until caught
        unsigned waitTicks = 0;
        while (sim.active && waitTicks < 150)
        {
            sim.Tick(false);
            ++waitTicks;
        }
        assert(!sim.active);
        assert(!sim.unarmed);
        assert(sim.constraint4Attached);
        assert(sim.constraint4Bone == 1792);
        assert(sim.handAttachState == 1);
        assert(sim.playerRno0 == 0x100000);
    }

    // 5. Verify Failsafe Timeout (Catch forced at tick 260 even if player teleports away)
    {
        RoundTripSim sim;
        sim.Throw();
        // Move player 500 units away so distance threshold is never reached naturally
        sim.playerPos.z = 500.0f;

        for (unsigned i = 0; i < 270 && sim.active; ++i)
            sim.Tick(false);

        // At tick 260, timeout must force Catch()
        assert(!sim.active);
        assert(!sim.unarmed);
        assert(sim.constraint4Attached);
        assert(sim.constraint4Bone == 1792);
        assert(sim.handAttachState == 1);
    }

    // 6. Verify Ultimate Queue Lockout during Round Trip
    {
        using namespace SamUltimatePolicy;
        Queue q;
        q.Request();
        assert(q.pending);

        // Simulating manager check: if (m_roundTripActive || !m_ultimateQueue.pending) return;
        bool roundTripActive = true;
        bool canExecuteUltimate = (!roundTripActive && q.pending && CanStart(0x100000, true, false, true));
        assert(!canExecuteUltimate); // Locked out while blade is in flight

        roundTripActive = false;
        canExecuteUltimate = (!roundTripActive && q.pending && CanStart(0x100000, true, false, true));
        assert(canExecuteUltimate); // Allowed once blade is caught
    }

    std::cout << "PASS: Round Trip throw/catch lifecycle, unarmed mode dispatch, dynamic homing, multi-hit damage, early recall and failsafe timeout\n";
    return 0;
}
