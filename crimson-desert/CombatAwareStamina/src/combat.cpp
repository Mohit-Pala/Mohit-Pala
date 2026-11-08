#include "combat.h"

#include <atomic>
#include <thread>
#include <windows.h>
#include <xinput.h>

#include "log.h"
#include "mem.h"
#include "scanner.h"

namespace cas::combat
{
    namespace
    {
        Config g_config;
        std::atomic<std::uint64_t> g_lastSignal{0};
        std::atomic<bool> g_running{false};
        std::thread g_poller;

        // Experimental game-side flag: either the address of the struct
        // pointer (deref) or the struct itself.
        std::uintptr_t g_flagSite = 0;

        using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
        XInputGetStateFn g_xinputGetState = nullptr;

        void Signal() { g_lastSignal.store(GetTickCount64(), std::memory_order_relaxed); }

        bool GameIsForeground()
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &pid);
            return pid == GetCurrentProcessId();
        }

        void LoadXInput()
        {
            for (const wchar_t* name : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"})
            {
                if (HMODULE module = LoadLibraryW(name))
                {
                    g_xinputGetState = reinterpret_cast<XInputGetStateFn>(reinterpret_cast<void*>(GetProcAddress(module, "XInputGetState")));
                    if (g_xinputGetState)
                        return;
                }
            }
        }

        void PollInputs()
        {
            while (g_running.load(std::memory_order_relaxed))
            {
                if (g_config.inputSignal && GameIsForeground())
                {
                    bool pressed = false;
                    for (int vk : g_config.attackKeys)
                        pressed |= (GetAsyncKeyState(vk) & 0x8000) != 0;

                    if (!pressed && g_xinputGetState && g_config.gamepadButtons != 0)
                    {
                        XINPUT_STATE state{};
                        if (g_xinputGetState(0, &state) == ERROR_SUCCESS)
                            pressed = (state.Gamepad.wButtons & g_config.gamepadButtons) != 0;
                    }

                    if (pressed)
                        Signal();
                }
                Sleep(10);
            }
        }

        void ResolveFlag()
        {
            if (g_config.flagPattern.empty())
                return;

            const auto hit = scan::Find(g_config.flagPattern);
            if (hit.matches != 1)
            {
                CAS_LOG("combat flag: pattern matched %d times (need exactly 1), flag disabled", hit.matches);
                return;
            }
            const std::uintptr_t disp = hit.address + g_config.flagRipOffset;
            g_flagSite = disp + 4 + *reinterpret_cast<const std::int32_t*>(disp);
            CAS_LOG("combat flag: resolved site 0x%p (deref=%d, byte +0x%X)",
                    reinterpret_cast<void*>(g_flagSite), g_config.flagDeref ? 1 : 0, g_config.flagByteOffset);
        }

        bool ReadFlag()
        {
            if (g_flagSite == 0)
                return false;
            std::uintptr_t base = g_flagSite;
            if (g_config.flagDeref && !mem::Read(g_flagSite, base))
                return false;
            std::uint8_t value = 0;
            return mem::Read(base + g_config.flagByteOffset, value) && value == 1;
        }
    }

    void Start(const Config& config)
    {
        g_config = config;
        ResolveFlag();
        LoadXInput();
        g_running = true;
        g_poller = std::thread(PollInputs);
    }

    void Stop()
    {
        g_running = false;
        if (g_poller.joinable())
            g_poller.join();
    }

    void NoteDamage()
    {
        if (g_config.damageSignal)
            Signal();
    }

    bool InCombat()
    {
        if (ReadFlag())
            return true;
        const std::uint64_t last = g_lastSignal.load(std::memory_order_relaxed);
        return last != 0 && GetTickCount64() - last < static_cast<std::uint64_t>(g_config.timeoutMs);
    }
}
