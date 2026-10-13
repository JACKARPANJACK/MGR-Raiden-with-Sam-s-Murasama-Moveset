#pragma once
#include <windows.h>
#include <string>
#include <algorithm>
#include "SamArchiveLookup.h"

namespace SamConfig
{
    inline std::string GetIniPath()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        char* slash = std::strrchr(path, '\\');
        if (slash) std::strcpy(slash + 1, "SamMoveset.ini");
        return std::string(path);
    }

    inline int GetInt(const char* section, const char* key, int defaultValue)
    {
        return GetPrivateProfileIntA(section, key, defaultValue, GetIniPath().c_str());
    }

    inline std::string GetString(const char* section, const char* key, const char* defaultValue)
    {
        char buffer[256];
        GetPrivateProfileStringA(section, key, defaultValue, buffer, sizeof(buffer), GetIniPath().c_str());
        return std::string(buffer);
    }
}

#include "SamUltimatePolicy.h"
#include "SamDirectionalPolicy.h"
#include <sstream>

namespace SamConfig
{
    inline void LoadConfig()
    {
        // Directional Moves
        const char* dirs[] = {"None", "Forward", "Back", "Left", "Right"};
        for (int i = 1; i <= 4; ++i)
        {
            SamDirectionalPolicy::heldLight[i] = GetInt("DirectionalMoves", (std::string(dirs[i]) + "Light").c_str(), SamDirectionalPolicy::heldLight[i]);
            SamDirectionalPolicy::heldHeavy[i] = GetInt("DirectionalMoves", (std::string(dirs[i]) + "Heavy").c_str(), SamDirectionalPolicy::heldHeavy[i]);
            SamDirectionalPolicy::flickLight[i] = GetInt("DirectionalMoves", (std::string(dirs[i]) + "FlickLight").c_str(), SamDirectionalPolicy::flickLight[i]);
            SamDirectionalPolicy::flickHeavy[i] = GetInt("DirectionalMoves", (std::string(dirs[i]) + "FlickHeavy").c_str(), SamDirectionalPolicy::flickHeavy[i]);
        }

        // Custom Moves
        int customMovesCount = GetInt("Moves", "Count", 0);
        if (customMovesCount > 0)
        {
            SamUltimatePolicy::Moves.clear();
            for (int i = 0; i < customMovesCount; ++i)
            {
                std::string prefix = "Move" + std::to_string(i) + "_";
                std::string name = GetString("Moves", (prefix + "Name").c_str(), "Unknown");
                std::string windup = GetString("Moves", (prefix + "Windup").c_str(), "");
                std::string release = GetString("Moves", (prefix + "Release").c_str(), "");
                bool raiden = GetInt("Moves", (prefix + "Raiden").c_str(), 0) != 0;
                bool playable = GetInt("Moves", (prefix + "Playable").c_str(), 0) != 0;
                SamUltimatePolicy::Move move{name, windup, release, raiden, playable};
                std::stringstream followups(GetString("Moves", (prefix + "Followups").c_str(), ""));
                std::string stage;
                while (std::getline(followups, stage, ','))
                    if (SamArchiveLookup::ValidCode(stage.c_str())) move.followups.push_back(stage);
                SamUltimatePolicy::Moves.push_back(std::move(move));
            }
        }
        // This reserved light continuation is part of the combat graph. Older
        // INIs list only 19 specials and must not remove it when loading.
        if(SamUltimatePolicy::Moves.size()<=19)SamUltimatePolicy::Moves.resize(20);
        SamUltimatePolicy::Moves[19]=SamUltimatePolicy::LightExtension();
        for (int direction = 1; direction <= 4; ++direction)
            for (int* table : {SamDirectionalPolicy::heldLight, SamDirectionalPolicy::heldHeavy,
                SamDirectionalPolicy::flickLight, SamDirectionalPolicy::flickHeavy})
                if (table[direction] < -1 || table[direction] >= static_cast<int>(SamUltimatePolicy::Count()))
                    table[direction] = -1;

        // Ultimates
        std::string ultStr = GetString("Moves", "UltimateIndices", "");
        if (!ultStr.empty())
        {
            SamUltimatePolicy::UltimateIndices.clear();
            std::stringstream ss(ultStr);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                try {
                    const int index = std::stoi(token);
                    if (index >= 0 && static_cast<unsigned>(index) < SamUltimatePolicy::Count())
                        SamUltimatePolicy::UltimateIndices.push_back(static_cast<unsigned>(index));
                } catch (...) {}
            }
        }
    }
}
