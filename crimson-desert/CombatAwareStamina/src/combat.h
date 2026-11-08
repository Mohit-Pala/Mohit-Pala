#pragma once
#include <cstdint>

#include "config.h"

namespace cas::combat
{
    // Resolves the optional game-side flag and starts the input poller.
    void Start(const Config& config);
    void Stop();

    // Called from the stat-write hook whenever any actor loses health.
    void NoteDamage();

    bool InCombat();
}
