#pragma once

#include "logging.h"

#include <initializer_list>
#include <source_location>

using byte = unsigned char;

namespace reverse_engineering
{
    using signature_byte = short;

    enum : signature_byte
    {
        any_byte = -1,
    };

    class signature
    {
    public:
        constexpr static const size_t CAPACITY = 64;

        constexpr signature(std::initializer_list<signature_byte> init) :
            size_(init.size())
        {
            for (size_t i = 0; i < CAPACITY; ++i)
                signature_[i] = (i < init.size()) ? *(init.begin() + i) : any_byte;
        }

        signature_byte operator[](size_t offset) const { return signature_[offset]; }

        operator bool() const { return info_; }
        bool operator!() const { return !info_; }

        [[nodiscard]] size_t size() const { return size_; }
        //byte* data() const { return data_; }

        [[nodiscard]] uintptr_t get_offset() const;
        [[nodiscard]] uintptr_t get_address() const;

        [[nodiscard]] uintptr_t get_4byte_displacement(const char* id, size_t offset, std::source_location source_loc = std::source_location::current()) const;
        [[nodiscard]] uintptr_t get_2byte_displacement(const char* id, size_t offset, std::source_location source_loc = std::source_location::current()) const;
        [[nodiscard]] uintptr_t get_1byte_displacement(const char* id, size_t offset, std::source_location source_loc = std::source_location::current()) const;

        template<typename T>
        [[nodiscard]] T get_value(const char* id, size_t offset, std::source_location source_loc = std::source_location::current()) const
        {
            T value = *reinterpret_cast<const T*>(data_ + offset);

            plugin_log::log(source_loc, spdlog::level::info,
                "'{0}' {1}-byte value at offset 0x{2:X} (0x{3:X}) equals: {4} (0x{4:X})"sv, id, sizeof(T), get_offset() + offset, get_address() + offset, value);

            return value;
        }

        void memory_write(uintptr_t offset, byte* buffer, size_t size) const;
        void memory_write(uintptr_t offset, byte byte) const;
        void memory_write_nop(uintptr_t offset, size_t size = 1) const;

    private:
        friend class info;

        mutable const info* info_ = nullptr;
        mutable const byte* data_ = nullptr;
        size_t size_;
        signature_byte signature_[CAPACITY];
    };

    class info
    {
    public:
        using snapshot_buffer = std::vector<uint8_t>;

        bool read_process(std::source_location source_loc = std::source_location::current());

        [[nodiscard]] uintptr_t get_base_address() const { return base_; }
        [[nodiscard]] const snapshot_buffer& get_snapshot_buffer() const { return snapshot_; }

        bool find_signature(const char* id, const signature& sig, std::source_location source_loc = std::source_location::current()) const;
        bool find_signature(const signature& sig) const;

    private:
        uintptr_t base_;
        snapshot_buffer snapshot_;
    };

    void memory_write(uintptr_t address, byte* buffer, size_t size);
    void memory_write(uintptr_t address, byte byte);
    void memory_write_nop(uintptr_t address, size_t size = 1);
}
