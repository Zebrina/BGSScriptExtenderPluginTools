// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Exclude rarely-used stuff from Windows headers.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj_core.h>
#include <TlHelp32.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#ifndef NDEBUG
#include <spdlog/sinks/msvc_sink.h>
#endif

#include <tomlplusplus/toml.hpp>

#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

#endif //PCH_H
