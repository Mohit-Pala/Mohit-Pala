#include "scanner.h"

#include <cstdlib>
#include <optional>
#include <vector>
#include <windows.h>

namespace cas::scan
{
    namespace
    {
        std::vector<std::optional<std::uint8_t>> Parse(std::string_view pattern)
        {
            std::vector<std::optional<std::uint8_t>> bytes;
            for (size_t i = 0; i < pattern.size();)
            {
                if (pattern[i] == ' ')
                {
                    ++i;
                    continue;
                }
                if (pattern[i] == '?')
                {
                    bytes.emplace_back(std::nullopt);
                    while (i < pattern.size() && pattern[i] == '?')
                        ++i;
                    continue;
                }
                if (i + 1 >= pattern.size())
                    return {};
                const char hex[3] = {pattern[i], pattern[i + 1], 0};
                char* end = nullptr;
                const unsigned long value = std::strtoul(hex, &end, 16);
                if (end != hex + 2)
                    return {};
                bytes.emplace_back(static_cast<std::uint8_t>(value));
                i += 2;
            }
            return bytes;
        }
    }

    Result Find(std::string_view pattern)
    {
        Result result;
        const auto bytes = Parse(pattern);
        if (bytes.empty())
            return result;

        auto* const base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);

        for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s, ++section)
        {
            if ((section->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
                continue;

            const std::uint8_t* start = base + section->VirtualAddress;
            const size_t size = section->Misc.VirtualSize;
            if (size < bytes.size())
                continue;

            for (size_t i = 0; i <= size - bytes.size(); ++i)
            {
                bool hit = true;
                for (size_t k = 0; k < bytes.size(); ++k)
                {
                    if (bytes[k] && start[i + k] != *bytes[k])
                    {
                        hit = false;
                        break;
                    }
                }
                if (!hit)
                    continue;

                if (result.matches++ == 0)
                    result.address = reinterpret_cast<std::uintptr_t>(start + i);
                if (result.matches > 1)
                    return result;
            }
        }
        return result;
    }
}
