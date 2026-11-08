#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace cas
{
    struct Config
    {
        // [General]
        bool enabled = true;
        int toggleKey = 0x77; // F8. 0 disables the hotkey.

        // [Stamina] percentages of the game's own amounts. 100 = vanilla.
        int outOfCombatCostPercent = 0;   // 0 = no stamina used outside combat
        int inCombatCostPercent = 100;    // leave combat untouched by default
        int outOfCombatRegenPercent = 100;

        // [Combat] how "in combat" is decided. Any signal counts; combat ends
        // TimeoutMs after the last one.
        int timeoutMs = 6000;
        bool damageSignal = true;   // any health loss on any actor
        std::int64_t minDamage = 1; // ignore health losses smaller than this
        bool inputSignal = true;    // attack / guard inputs
        std::vector<int> attackKeys{0x01, 0x02}; // VK codes, default LMB + RMB
        std::uint16_t gamepadButtons = 0xC200;   // XInput mask: X + Y + RB

        // Experimental: a game-side combat byte. Off unless a pattern is given.
        std::string flagPattern;
        int flagRipOffset = 3;
        int flagByteOffset = 0x1A;
        bool flagDeref = true; // the RIP target is a pointer to the struct, not the struct

        // [Hook] the shared stat-write commit (health, stamina, spirit all go through it).
        std::string statWritePattern =
            "48 2B 47 18 48 39 5F 18 48 0F 4F C2 48 89 47 20 48 FF 47 48 48 89 5F 08 48 8B 5C 24 48 48 89 77 38 66 89 6F 50";
        int statWriteOffset = 20;
        std::string statWriteFallbackPattern = "48 FF 47 48 48 89 5F 08 48 8B 5C 24 48 48 89 77 38";
        int statWriteFallbackOffset = 4;
        int staminaStatId = 17;
        int healthStatId = 0;

        // [Debug]
        bool verbose = false;
    };

    // Reads the INI. Missing keys (or a missing file) fall back to the defaults above.
    Config LoadConfig(const std::filesystem::path& ini);
}
