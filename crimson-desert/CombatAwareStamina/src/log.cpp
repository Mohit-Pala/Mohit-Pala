#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <windows.h>

namespace cas::log
{
    namespace
    {
        std::mutex g_mutex;
        FILE* g_file = nullptr;
    }

    void Init(const std::filesystem::path& file)
    {
        std::scoped_lock lock(g_mutex);
        if (g_file == nullptr)
            g_file = _wfopen(file.c_str(), L"w");
    }

    void Write(const char* fmt, ...)
    {
        char line[1024];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(line, sizeof(line), fmt, args);
        va_end(args);

        SYSTEMTIME t;
        GetLocalTime(&t);

        std::scoped_lock lock(g_mutex);
        if (g_file == nullptr)
            return;
        std::fprintf(g_file, "[%02u:%02u:%02u.%03u] %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, line);
        std::fflush(g_file);
    }
}
