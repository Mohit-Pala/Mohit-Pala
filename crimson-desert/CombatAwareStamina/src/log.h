#pragma once
#include <filesystem>

namespace cas::log
{
    void Init(const std::filesystem::path& file);
    void Write(const char* fmt, ...);
}

#define CAS_LOG(...) ::cas::log::Write(__VA_ARGS__)
