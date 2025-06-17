#include "logging.h"

//#include <fmt/format.h>

namespace plugin_log
{
    bool initialize(const std::string_view& name, const std::string& pattern)
    {
        CHAR directory[MAX_PATH];
        DWORD length = GetCurrentDirectoryA(MAX_PATH, directory);
        if (length == 0)
            return false;

        auto path = fmt::format("{}\\{}.log"sv, directory, name);

#ifndef NDEBUG
        auto vssink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#endif

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);

#ifndef NDEBUG
        spdlog::logger multilog("multi sink"s, { sink, vssink });
        auto log = std::make_shared<spdlog::logger>(std::move(multilog));

        //auto rotatinglog = spdlog::rotating_logger_mt("rotating log", path, 0, 5, true);
        //auto log = std::make_shared<spdlog::logger>(std::move(rotatinglog));

        const auto level = spdlog::level::trace;
        log->set_level(level);
        log->flush_on(level);
#else
        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

        const auto level = spdlog::level::info;
        log->set_level(level);
        log->flush_on(level);
#endif

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern(pattern);

        return true;
    }
}
