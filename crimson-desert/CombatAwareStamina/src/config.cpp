#include "config.h"

#include <cstdlib>
#include <sstream>
#include <windows.h>

namespace cas
{
    namespace
    {
        std::string ReadString(const std::filesystem::path& ini, const char* section, const char* key, const std::string& def)
        {
            char buffer[1024] = {};
            GetPrivateProfileStringA(section, key, def.c_str(), buffer, sizeof(buffer), ini.string().c_str());
            std::string value = buffer;
            // Strip trailing ";" comments and whitespace.
            if (const auto semi = value.find(';'); semi != std::string::npos)
                value.erase(semi);
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
                value.pop_back();
            return value;
        }

        long long ReadInt(const std::filesystem::path& ini, const char* section, const char* key, long long def)
        {
            const std::string value = ReadString(ini, section, key, "");
            if (value.empty())
                return def;
            char* end = nullptr;
            const long long parsed = std::strtoll(value.c_str(), &end, 0); // accepts 0x..
            return end == value.c_str() ? def : parsed;
        }

        int Clamp(long long v, int lo, int hi)
        {
            return static_cast<int>(v < lo ? lo : (v > hi ? hi : v));
        }

        std::vector<int> ParseKeyList(const std::string& text, const std::vector<int>& def)
        {
            if (text.empty())
                return def;
            std::vector<int> keys;
            std::stringstream stream(text);
            std::string item;
            while (std::getline(stream, item, ','))
            {
                char* end = nullptr;
                const long vk = std::strtol(item.c_str(), &end, 0);
                if (end != item.c_str() && vk > 0 && vk < 256)
                    keys.push_back(static_cast<int>(vk));
            }
            return keys;
        }
    }

    Config LoadConfig(const std::filesystem::path& ini)
    {
        Config c;
        const char* G = "General";
        const char* S = "Stamina";
        const char* C = "Combat";
        const char* H = "Hook";

        c.enabled = ReadInt(ini, G, "Enabled", c.enabled) != 0;
        c.toggleKey = Clamp(ReadInt(ini, G, "ToggleKey", c.toggleKey), 0, 255);

        c.outOfCombatCostPercent = Clamp(ReadInt(ini, S, "OutOfCombatCostPercent", c.outOfCombatCostPercent), 0, 100);
        c.inCombatCostPercent = Clamp(ReadInt(ini, S, "InCombatCostPercent", c.inCombatCostPercent), 0, 100);
        c.outOfCombatRegenPercent = Clamp(ReadInt(ini, S, "OutOfCombatRegenPercent", c.outOfCombatRegenPercent), 100, 10000);

        c.timeoutMs = Clamp(ReadInt(ini, C, "TimeoutMs", c.timeoutMs), 500, 120000);
        c.damageSignal = ReadInt(ini, C, "DamageSignal", c.damageSignal) != 0;
        c.minDamage = ReadInt(ini, C, "MinDamage", c.minDamage);
        c.inputSignal = ReadInt(ini, C, "InputSignal", c.inputSignal) != 0;
        c.attackKeys = ParseKeyList(ReadString(ini, C, "AttackKeys", ""), c.attackKeys);
        c.gamepadButtons = static_cast<std::uint16_t>(ReadInt(ini, C, "GamepadButtons", c.gamepadButtons));
        c.flagPattern = ReadString(ini, C, "FlagPattern", "");
        c.flagRipOffset = Clamp(ReadInt(ini, C, "FlagRipOffset", c.flagRipOffset), 0, 64);
        c.flagByteOffset = Clamp(ReadInt(ini, C, "FlagByteOffset", c.flagByteOffset), 0, 0x10000);
        c.flagDeref = ReadInt(ini, C, "FlagDeref", c.flagDeref) != 0;

        c.statWritePattern = ReadString(ini, H, "StatWritePattern", c.statWritePattern);
        c.statWriteOffset = Clamp(ReadInt(ini, H, "StatWriteOffset", c.statWriteOffset), 0, 256);
        c.statWriteFallbackPattern = ReadString(ini, H, "StatWriteFallbackPattern", c.statWriteFallbackPattern);
        c.statWriteFallbackOffset = Clamp(ReadInt(ini, H, "StatWriteFallbackOffset", c.statWriteFallbackOffset), 0, 256);
        c.staminaStatId = Clamp(ReadInt(ini, H, "StaminaStatId", c.staminaStatId), 0, 4096);
        c.healthStatId = Clamp(ReadInt(ini, H, "HealthStatId", c.healthStatId), 0, 4096);

        c.verbose = ReadInt(ini, "Debug", "Verbose", c.verbose) != 0;
        return c;
    }
}
