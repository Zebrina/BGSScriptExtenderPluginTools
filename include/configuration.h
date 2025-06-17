#pragma once

#include "logging.h"

using namespace std::literals;

class plugin_configuration
{
public:
    plugin_configuration(const std::string_view& configFilePath);
    plugin_configuration(const plugin_configuration&) = delete;
    plugin_configuration(plugin_configuration&&) = delete;
    ~plugin_configuration() { close(); }

    [[nodiscard]] const char* get_filename() const { return pluginFileName_.data(); }
    template <typename T>
    [[nodiscard]] T get(const std::string_view section, const std::string_view key, T defaultValue, std::source_location source_loc = std::source_location::current())
    {
        T value = get_table()[section][key].value_or(defaultValue);

        if (value == defaultValue)
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} = {} (default)"sv, configFilePath_, section, key, value);
        }
        else
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} = {}"sv, configFilePath_, section, key, value);
        }

        return value;
    }
#ifdef COMMONLIB_STUFF
    template <typename T = RE::TESForm>
    [[nodiscard]] T* GetForm(const std::string_view key, RE::FormID defaultFormID = 0x0)
    {
        RE::TESDataHandler* data = RE::TESDataHandler::GetSingleton();

        assert(data);

        RE::FormID formID = GetTable()["Forms"sv][key].value_or(defaultFormID);
        T* form = data->LookupForm<T>(formID, pluginFileName_);
        if (!form)
        {
            SKSE::log::error("Plugin file '{}' not enabled or {} form id is no longer XX{:06X}!"sv, pluginFileName_, key, formID);
            throw std::exception();
        }
        return form;
    }
#endif

    template <typename T>
    void set(const std::string_view section, const std::string_view key, T value)
    {
        get_subtable(section).insert_or_assign(key, value);
        modified_ = true;
    }
#ifdef COMMONLIB_STUFF
    template<>
    void Set<RE::FormID>(const std::string_view section, const std::string_view key, RE::FormID value)
    {
        GetOrInsertTable(section).insert_or_assign(section, tomltable{ { key, value } }, toml::value_flags::format_as_hexadecimal);
        modified_ = true;
    }
    void SetForm(const std::string_view& key, RE::FormID value)
    {
        Set<RE::FormID>("Forms"sv, key, value);
    }
#endif

    [[nodiscard]] toml::table& get_table();
    [[nodiscard]] toml::table& get_subtable(const std::string_view& section);
    void close();

private:
    std::string_view configFilePath_;
    std::string_view pluginFileName_;
    toml::table* table_{ nullptr };
    bool modified_{ false };
};
