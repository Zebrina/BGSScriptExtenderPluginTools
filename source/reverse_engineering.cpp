#include "reverse_engineering.h"

#include "logging.h"

namespace reverse_engineering
{
    uintptr_t signature::get_offset() const
    {
        return data_ - info_->get_snapshot_buffer().data();
    }

    uintptr_t signature::get_address() const
    {
        return info_->get_base_address() + get_offset();
    }

    [[nodiscard]] uintptr_t signature::get_4byte_displacement(const char* id, size_t offset, std::source_location source_loc) const
    {
        if (offset > size_)
            return NULL;

        uintptr_t address = info_->get_base_address() + get_offset() + offset + sizeof(int) + *(int*)(data_ + offset);

        plugin_log::log(source_loc, spdlog::level::info,
            "'{}' resolved to offset 0x{:X} (0x{:X})"sv, id, address - info_->get_base_address(), address);

        return address;
    }

    [[nodiscard]] uintptr_t signature::get_2byte_displacement(const char* id, size_t offset, std::source_location source_loc) const
    {
        if (offset > size_)
            return NULL;

        uintptr_t address = info_->get_base_address() + get_offset() + offset + sizeof(short) + *(short*)(data_ + offset);

        plugin_log::log(source_loc, spdlog::level::info,
            "'{}' resolved to offset 0x{:X} (0x{:X})"sv, id, address - info_->get_base_address(), address);

        return address;
    }

    [[nodiscard]] uintptr_t signature::get_1byte_displacement(const char* id, size_t offset, std::source_location source_loc) const
    {
        if (offset > size_)
            return NULL;

        uintptr_t address = info_->get_base_address() + get_offset() + offset + sizeof(char) + *(char*)(data_ + offset);

        plugin_log::log(source_loc, spdlog::level::info,
            "'{}' resolved to offset 0x{:X} (0x{:X})"sv, id, address - info_->get_base_address(), address);

        return address;
    }

    void signature::memory_write(uintptr_t offset, byte* buffer, size_t size) const
    {
        uintptr_t address = get_address() + offset;
        DWORD oldProtect;
        VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memcpy((void*)address, buffer, size);
        VirtualProtect((void*)address, size, oldProtect, &oldProtect);
    }

    void signature::memory_write(uintptr_t offset, byte byte) const
    {
        constexpr const size_t SIZE = sizeof(byte);
        uintptr_t address = get_address() + offset;
        DWORD oldProtect;
        VirtualProtect((void*)address, SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memcpy((void*)address, &byte, SIZE);
        VirtualProtect((void*)address, SIZE, oldProtect, &oldProtect);
    }

    void signature::memory_write_nop(uintptr_t offset, size_t size) const
    {
        uintptr_t address = get_address() + offset;
        DWORD oldProtect;
        VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memset((void*)address, 0x90, size);
        VirtualProtect((void*)address, size, oldProtect, &oldProtect);
    }

    bool info::read_process(std::source_location source_loc)
    {
        HMODULE exeHModule = GetModuleHandle(NULL);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
        MODULEENTRY32 entry{ 0 };
        entry.dwSize = sizeof(MODULEENTRY32);

        int attempt = 0;
        constexpr size_t NUM_RETRY_COUNT = 3;
        while (!Module32First(hSnapshot, &entry))
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot
            // "If the function fails with ERROR_BAD_LENGTH, retry the function until it succeeds."
            DWORD error = GetLastError();
            if (error == ERROR_BAD_LENGTH && attempt < NUM_RETRY_COUNT)
            {
                plugin_log::log(source_loc, spdlog::level::err,
                    "Module32First failed with error ERROR_BAD_LENGTH. Retrying {}/{}"sv, ++attempt, NUM_RETRY_COUNT);

                continue;
            }

            CloseHandle(hSnapshot);
            return false;
        }

        DWORD size = 0;

        do
        {
            if (entry.hModule == exeHModule)
            {
                base_ = (uintptr_t)entry.modBaseAddr;
                size = entry.modBaseSize;
                CloseHandle(hSnapshot);
                break;
            }
        } while (Module32Next(hSnapshot, &entry));

        if (size == 0)
        {
            plugin_log::log(source_loc, spdlog::level::critical, "Exe module not found."sv);
            return false;
        }

        snapshot_.resize(size);

        BOOL readSuccess = ReadProcessMemory(GetCurrentProcess(), (LPCVOID)base_, snapshot_.data(), size, nullptr);

        if (readSuccess == FALSE)
        {
            plugin_log::log(source_loc, spdlog::level::err,
                "ReadProcessMemory failed with error code {0} (0x{0:X})"sv, GetLastError());
            snapshot_.clear();
        }

        return readSuccess;
    }

    bool info::find_signature(const char* id, const signature& sig,
        std::source_location source_loc) const
    {
        bool success = find_signature(sig);

        if (success)
        {
            uintptr_t offset = sig.get_offset();
            plugin_log::log(source_loc, spdlog::level::info,
                "Found '{}' signature at offset 0x{:X} (0x{:X})"sv, id, offset, base_ + offset);
        }
        else
        {
            plugin_log::log(source_loc, spdlog::level::err,
                "Unable to find '{}' signature."sv, id);
        }

        return success;
    }

    bool info::find_signature(const signature& sig) const
    {
        for (size_t i = 0, n = snapshot_.size() - sig.size(); i < n; ++i)
        {
            size_t j = 0, m = sig.size();
            for (; j < m; ++j)
            {
                if (sig[j] != -1 && sig[j] != snapshot_[i + j])
                    break;
            }

            if (j == m)
            {
                sig.info_ = this;
                sig.data_ = snapshot_.data() + i;
                return true;
            }
        }
        return false;
    }

    void memory_write(uintptr_t address, byte* buffer, size_t size)
    {
        DWORD oldProtect;
        VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memcpy((void*)address, buffer, size);
        VirtualProtect((void*)address, size, oldProtect, &oldProtect);
    }

    void memory_write(uintptr_t address, byte byte)
    {
        constexpr const size_t SIZE = sizeof(byte);
        DWORD oldProtect;
        VirtualProtect((void*)address, SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memcpy((void*)address, &byte, SIZE);
        VirtualProtect((void*)address, SIZE, oldProtect, &oldProtect);
    }

    void memory_write_nop(uintptr_t address, size_t size)
    {
        DWORD oldProtect;
        VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memset((void*)address, 0x90, size);
        VirtualProtect((void*)address, size, oldProtect, &oldProtect);
    }
}
