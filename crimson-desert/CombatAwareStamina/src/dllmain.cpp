// CombatAwareStamina - reduce or remove stamina costs only while out of combat.
//
// One mid-function hook on the game's shared stat-write commit, the place where
// a new Health / Stamina / Spirit value is stored into a stat entry:
//
//     mov [rdi+0x08], rbx      ; rdi = stat entry, rbx = new value
//
// Stat entry layout (community RE, see docs/RESEARCH.md):
//     +0x00 int32  stat id   (0 = Health, 17 = Stamina, 18 = Spirit)
//     +0x08 int64  current
//     +0x18 int64  max
//
// Health losses on any actor feed the combat detector. Stamina writes are
// rescaled depending on whether we are in combat.

#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <windows.h>

#include <safetyhook.hpp>

#include "combat.h"
#include "config.h"
#include "log.h"
#include "mem.h"
#include "scanner.h"

namespace
{
    cas::Config g_config;
    std::atomic<bool> g_enabled{true};
    std::atomic<bool> g_stop{false};
    SafetyHookMid g_statWriteHook;
    std::atomic<bool> g_wasInCombat{false};
    std::atomic<int> g_verboseBudget{200};

    std::int64_t Scale(std::int64_t amount, int percent)
    {
        return amount * percent / 100;
    }

    std::int64_t Clamp(std::int64_t v, std::int64_t lo, std::int64_t hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    void OnStatWrite(SafetyHookContext& ctx)
    {
        const std::uintptr_t entry = ctx.rdi;
        std::int32_t id = 0;
        std::int64_t current = 0;
        std::int64_t max = 0;
        if (!cas::mem::Read(entry + 0x00, id) || !cas::mem::Read(entry + 0x08, current) ||
            !cas::mem::Read(entry + 0x18, max))
            return;

        const auto requested = static_cast<std::int64_t>(ctx.rbx);
        if (max <= 0 || current < 0 || current > max || requested < 0)
            return;

        if (id == g_config.healthStatId)
        {
            if (requested < current && current - requested >= g_config.minDamage)
                cas::combat::NoteDamage();
            return;
        }

        if (id != g_config.staminaStatId || !g_enabled.load(std::memory_order_relaxed))
            return;

        const std::int64_t delta = requested - current;
        if (delta == 0)
            return;

        const bool inCombat = cas::combat::InCombat();
        if (inCombat != g_wasInCombat.exchange(inCombat, std::memory_order_relaxed))
            CAS_LOG("state: %s", inCombat ? "IN COMBAT" : "out of combat");

        std::int64_t adjusted = requested;
        if (delta < 0)
        {
            const int percent = inCombat ? g_config.inCombatCostPercent : g_config.outOfCombatCostPercent;
            if (percent != 100)
                adjusted = current + Scale(delta, percent);
        }
        else if (!inCombat && g_config.outOfCombatRegenPercent != 100)
        {
            adjusted = current + Scale(delta, g_config.outOfCombatRegenPercent);
        }

        adjusted = Clamp(adjusted, 0, max);
        if (adjusted == requested)
            return;

        if (g_config.verbose && g_verboseBudget.fetch_sub(1, std::memory_order_relaxed) > 0)
            CAS_LOG("stamina entry=0x%p cur=%lld req=%lld -> %lld max=%lld combat=%d", reinterpret_cast<void*>(entry),
                    static_cast<long long>(current), static_cast<long long>(requested),
                    static_cast<long long>(adjusted), static_cast<long long>(max), inCombat ? 1 : 0);

        ctx.rbx = static_cast<std::uintptr_t>(adjusted);
    }

    std::uintptr_t FindStatWrite()
    {
        struct Candidate
        {
            const char* name;
            const std::string* pattern;
            int offset;
        };
        const Candidate candidates[] = {
            {"primary", &g_config.statWritePattern, g_config.statWriteOffset},
            {"fallback", &g_config.statWriteFallbackPattern, g_config.statWriteFallbackOffset},
        };

        for (const auto& c : candidates)
        {
            if (c.pattern->empty())
                continue;
            const auto hit = cas::scan::Find(*c.pattern);
            CAS_LOG("scan: stat-write %s pattern matched %d time(s)", c.name, hit.matches);
            if (hit.matches == 1)
                return hit.address + c.offset;
        }
        return 0;
    }

    void Worker(std::filesystem::path dir)
    {
        cas::log::Init(dir / "CombatAwareStamina.log");
        g_config = cas::LoadConfig(dir / "CombatAwareStamina.ini");
        g_enabled = g_config.enabled;
        CAS_LOG("CombatAwareStamina loaded. outOfCombat=%d%% inCombat=%d%% regen=%d%% timeout=%dms",
                g_config.outOfCombatCostPercent, g_config.inCombatCostPercent, g_config.outOfCombatRegenPercent,
                g_config.timeoutMs);

        // Code can still be unpacking right after load; give it up to a minute.
        std::uintptr_t target = 0;
        for (int attempt = 0; attempt < 60 && target == 0 && !g_stop; ++attempt)
        {
            target = FindStatWrite();
            if (target == 0)
                Sleep(1000);
        }
        if (target == 0)
        {
            CAS_LOG("FATAL: stat-write hook site not found. The game was probably patched - update the [Hook] patterns.");
            return;
        }

        // The hook site must be the store we expect: mov [rdi+0x08], rbx.
        const auto* site = reinterpret_cast<const std::uint8_t*>(target);
        if (!(site[0] == 0x48 && site[1] == 0x89 && site[2] == 0x5F && site[3] == 0x08))
        {
            CAS_LOG("FATAL: hook site 0x%p is not `mov [rdi+8], rbx` (%02X %02X %02X %02X). Refusing to hook.",
                    reinterpret_cast<void*>(target), site[0], site[1], site[2], site[3]);
            return;
        }

        cas::combat::Start(g_config);

        auto hook = safetyhook::MidHook::create(reinterpret_cast<void*>(target), OnStatWrite);
        if (!hook)
        {
            CAS_LOG("FATAL: failed to create stat-write hook");
            return;
        }
        g_statWriteHook = std::move(*hook);
        CAS_LOG("hooked stat-write at 0x%p (exe+0x%llX)", reinterpret_cast<void*>(target),
                static_cast<unsigned long long>(target - reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))));

        bool keyWasDown = false;
        while (!g_stop)
        {
            if (g_config.toggleKey != 0)
            {
                const bool down = (GetAsyncKeyState(g_config.toggleKey) & 0x8000) != 0;
                if (down && !keyWasDown)
                {
                    const bool now = !g_enabled.load();
                    g_enabled = now;
                    CAS_LOG("toggled %s", now ? "ON" : "OFF");
                    Beep(now ? 880 : 440, 80);
                }
                keyWasDown = down;
            }
            Sleep(50);
        }
    }

    std::filesystem::path ModuleDir(HMODULE module)
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(module, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        std::thread(Worker, ModuleDir(module)).detach();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        g_stop = true;
    }
    return TRUE;
}
