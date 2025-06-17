#pragma once

#include "pch.h"

#include <spdlog/common.h>

#include <source_location>

#pragma push_macro("MAKE_LOGGER")

#define MAKE_LOGGER(type) \
    template <class... Args> \
    struct type \
    { \
        type() = delete; \
        explicit type(spdlog::format_string_t<Args...> format, Args&&... args, \
            std::source_location source_loc = std::source_location::current()) \
        { \
            spdlog::log( \
                spdlog::source_loc{ \
                    source_loc.file_name(), \
                    static_cast<int>(source_loc.line()), \
                    source_loc.function_name() }, \
                spdlog::level::type, \
                format, \
                std::forward<Args>(args)...); \
        } \
    }; \
    template <class... Args> \
    type(spdlog::format_string_t<Args...>, Args&&...) -> type<Args...>;

namespace plugin_log
{
    using namespace std::literals;

    bool initialize(const std::string_view& name, const std::string& pattern = "%s(%#): [%^%l%$] %v"s);

    template <class... Args>
    void log(const std::source_location& source_loc, spdlog::level::level_enum level,
        spdlog::format_string_t<Args...> format, Args&&... args)
    {
        spdlog::log(
            spdlog::source_loc{
                source_loc.file_name(),
                static_cast<int>(source_loc.line()),
                source_loc.function_name() },
            level,
            format,
            std::forward<Args>(args)...);
    }

    MAKE_LOGGER(trace);
    MAKE_LOGGER(debug);
    MAKE_LOGGER(info);
    MAKE_LOGGER(warn);
    MAKE_LOGGER(err);
    MAKE_LOGGER(critical);
}

#pragma pop_macro("MAKE_LOGGER")
