#pragma once

#include <cstdint>

namespace plugin_version
{
    using version_number_t = unsigned char;
    constexpr uint32_t make(uint8_t major, uint16_t minor, uint16_t patch, uint8_t build)
    {
        return (major << 28u) | ((minor & 0xFFF) << 16u) | ((patch & 0xFFF) << 4u) | build;
    }
}
