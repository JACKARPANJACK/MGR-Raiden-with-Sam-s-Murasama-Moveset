#pragma once
#include <windows.h>
#include <string>

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
                SamUltimatePolicy::Moves.push_back({name, windup, release, raiden});
            }
        }

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
                    SamUltimatePolicy::UltimateIndices.push_back(std::stoi(token));
                } catch (...) {}
            }
        }
    }
}
