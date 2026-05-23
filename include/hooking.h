#pragma once

#include "reverse_engineering.h"

#include <source_location>

namespace hooking
{
    inline uintptr_t get_vtbl(void* virtual_class_ptr)
    {
        return (uintptr_t) * (uintptr_t**)virtual_class_ptr;
    }

    template<typename T>
    T hook_function(uintptr_t address, T new_function)
    {
#ifdef OBSEAPI
        return (T)REL::GetTrampoline().write_call5(address, (uintptr_t)new_function);
#else
        throw std::logic_error("This function requires a script extender common library.");
#endif
    }
    template<typename T>
    T hook_function(const char* id, uintptr_t address, T new_function, std::source_location source_loc = std::source_location::current())
    {
        T old_function = hook_function(address, new_function);

        plugin_log::log(source_loc, spdlog::level::info,
            "'{}' hooked: 0x{:0>16X} => 0x{:0>16X}"sv, id, (uintptr_t)old_function, (uintptr_t)new_function);

        return old_function;
    }

    template<typename T>
    T hook_virtual_function(uintptr_t vtbl, T new_function, size_t index)
    {
        static_assert(sizeof(T) == sizeof(void*), "new_function must be a pointer to a function!");

        uintptr_t* vtbl_ptr = (uintptr_t*)vtbl;

        T old_function = (T)vtbl_ptr[index];

        uintptr_t new_function_ptr = (uintptr_t)new_function;

        reverse_engineering::memory_write((uintptr_t) & (vtbl_ptr[index]), (byte*)&new_function_ptr, sizeof(uintptr_t));

        return old_function;
    }
    template<typename T>
    T hook_virtual_function(const char* id, uintptr_t vtbl, T new_function, size_t index, std::source_location source_loc = std::source_location::current())
    {
        T old_function = hook_virtual_function(vtbl, new_function, index);

        plugin_log::log(source_loc, spdlog::level::info,
            "'{}' (0x{:X}) hooked: 0x{:0>16X} => 0x{:0>16X}"sv, id, index, (uintptr_t)old_function, (uintptr_t)new_function);

        return old_function;
    }
}
