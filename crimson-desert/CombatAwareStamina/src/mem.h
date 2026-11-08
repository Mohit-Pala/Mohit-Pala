#pragma once
#include <cstdint>
#include <cstring>
#include <windows.h>

namespace cas::mem
{
    inline bool Readable(std::uintptr_t address, size_t size)
    {
        if (address < 0x10000)
            return false;
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) == 0)
            return false;
        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
            return false;
        const auto end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
        return address + size <= end;
    }

    // Read that cannot crash the game. MSVC / clang-cl get SEH, everything
    // else falls back to a VirtualQuery check first.
    template <typename T>
    bool Read(std::uintptr_t address, T& out)
    {
#if defined(_MSC_VER)
        if (address < 0x10000)
            return false;
        __try
        {
            out = *reinterpret_cast<const volatile T*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
#else
        if (!Readable(address, sizeof(T)))
            return false;
        std::memcpy(&out, reinterpret_cast<const void*>(address), sizeof(T));
        return true;
#endif
    }
}
