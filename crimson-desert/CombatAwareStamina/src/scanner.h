#pragma once
#include <cstdint>
#include <string_view>

namespace cas::scan
{
    struct Result
    {
        std::uintptr_t address = 0; // first match, 0 if none
        int matches = 0;            // total matches in executable sections
    };

    // IDA-style pattern ("48 8B ?? 05 ? ?"), searched over every executable
    // section of the game's main module. Stops counting after 2 matches,
    // because all a caller needs to know is "unique or not".
    Result Find(std::string_view pattern);
}
