#pragma once

#include "logging.h"

#include <queue>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

class plugin_configuration
{
private:
    class directory_iterator
    {
    public:
        directory_iterator() = default;
        directory_iterator(const char* folder) :
            data_{ new data(folder) }
        {
            find_next();
        }
        directory_iterator(const directory_iterator&) = delete;
        directory_iterator(directory_iterator&&) = delete;
        ~directory_iterator() { destruct(); }

        bool is_valid() const { return data_; }
        plugin_configuration& get() const { return *(data_->current); }
        void find_next() { if (!data_->find_next()) { destruct(); } }

    private:
        struct data
        {
            data(const char* folder);
            ~data();

            bool find_next();

            const std::string folder;
            void* handle;
            plugin_configuration* current;
        };
        data* data_{ nullptr };

        void destruct();
    };

public:
    enum for_each_action
    {
        for_each_continue,
        for_each_yield,
        for_each_break,
    };

    class table_interface
    {
    public:
        table_interface(const std::string& name, toml::table* table) :
            name_{ name },
            table_{ table }
        { }
        table_interface(const table_interface&) = default;

        const char* get_name() const { return name_.c_str(); }

        toml::table& get_underlying_table() { return *table_; }

        template<typename T>
        [[nodiscard]] T get(const std::string_view key, T defaultValue)
        {
            return (*table_)[key].value_or(defaultValue);
        }

        template<typename T>
        [[nodiscard]] bool try_get(const std::string_view key, T& valueOut)
        {
            return internal_try_get<T>((*table_)[key], valueOut);
        }

        template<typename T>
        [[nodiscard]] std::vector<T> get_array(const std::string_view key)
        {
            return internal_get_array<T>((*table_)[key]);
        }

        template<typename FormType>
        [[nodiscard]] FormType* get_form(const std::string_view key)
        {
            return internal_get_form<FormType>((*table_)[key]);
        }

        template<typename FormType>
        [[nodiscard]] std::vector<FormType*> get_form_array(const std::string_view key)
        {
            return internal_get_form_array<FormType>((*table_)[key]);
        }

    private:
        std::string name_;
        toml::table* table_{ nullptr };
    };

    plugin_configuration(const std::string_view configFilePath);
    plugin_configuration(const plugin_configuration&) = delete;
    plugin_configuration(plugin_configuration&&) = delete;
    ~plugin_configuration() { close(); }

    template<typename ActionT>
    static void for_each_configuration(const std::string& folder, ActionT action/*, std::source_location source_loc = std::source_location::current()*/)
    {
        for (auto it = directory_iterator{ folder.c_str() }; it.is_valid(); it.find_next())
        {
            action(it.get());
        }
    }

    template<typename ActionT>
    void for_each_section(ActionT action/*, std::source_location source_loc = std::source_location::current()*/)
    {
        std::queue<table_interface> suspended{};

        for (auto& section : get_table())
        {
            if (!section.second.is_table())
                continue;

            table_interface table{ std::string{ section.first }, section.second.as_table() };

            for_each_action result = action(table);
            if (result == for_each_break)
                break;
            else if (result == for_each_yield)
                suspended.push(table);
        }

        while (!suspended.empty())
        {
            table_interface table = suspended.front();

            for_each_action result = action(table);
            if (result == for_each_break)
                break;
            else if (result == for_each_yield)
                suspended.push(table);

            suspended.pop();
        }
    }

    [[nodiscard]] const char* get_filename() const { return filePath_.c_str(); }

    template <typename T>
    [[nodiscard]] T get(const std::string_view section, const std::string_view key, T defaultValue, std::source_location source_loc = std::source_location::current())
    {
        T value = get_table()[section][key].value_or(defaultValue);

        if (value == defaultValue)
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} = {} (default)"sv, get_filename(), section, key, value);
        }
        else
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} = {}"sv, get_filename(), section, key, value);
        }

        return value;
    }

    template <typename T>
    [[nodiscard]] bool try_get(const std::string_view section, const std::string_view key, T& valueOut, std::source_location source_loc = std::source_location::current())
    {
        auto node = get_table()[section][key];
        bool success = internal_try_get(node, valueOut);

        if (success)
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} = {}"sv, get_filename(), section, key, valueOut);
        }
        else
        {
            plugin_log::log(source_loc, spdlog::level::info,
                "{}:{}:{} not found."sv, get_filename(), section, key);
        }

        return success;
    }

    template <typename FormType>
    [[nodiscard]] FormType* get_form(const std::string_view section, const std::string_view key, std::source_location source_loc = std::source_location::current())
    {
        auto node = get_table()[section][key];
        FormType* form = get_form(node, key, &source_loc);

        if (form == nullptr)
        {
            plugin_log::log(*source_loc, spdlog::level::err,
                "{}:{}:{} Form ID {} not found."sv, get_filename(), section, key, node.as_string()->get());
        }

        return form;
    }

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
    [[nodiscard]] toml::table& get_subtable(const std::string_view section);
    void close();

private:
    std::string filePath_;
    toml::table* table_{ nullptr };
    bool modified_{ false };

    static bool is_toml_file(const char* file);

    using toml_node_view = toml::node_view<toml::v3::node>;

    template<typename ValueType>
    static bool internal_try_get(toml_node_view node, ValueType& valueOut)
    {
        if (node.is<ValueType>())
        {
            ValueType value = *(node.value<ValueType>());
            valueOut = value;
            return true;
        }

        return false;
    }

    template<typename ValueType>
    static std::vector<ValueType> internal_get_array(toml_node_view node)
    {
        std::vector<ValueType> result{};

        if (node.is_array())
        {
            auto array = node.as_array();
            for (auto it = array->begin(); it != array->end(); ++it)
            {
                toml_node_view arrayNode{ *it };
                if (arrayNode.is<ValueType>())
                    result.push_back(arrayNode.as<ValueType>());
            }
        }
        else if (node.is<ValueType>())
        {
            result.push_back(node.as<ValueType>());
        }

        return result;
    }

    template<typename FormType>
    static FormType* internal_get_form(toml_node_view node)
    {
        std::uint32_t formID = 0;

        if (node.is_integer())
        {
            formID = (std::uint32_t)node.as_integer()->get();
        }
        else if (node.is_string())
        {
            std::string value = node.as_string()->get();

            formID = std::strtol(value.c_str(), nullptr, 16);
            if (formID == 0x0)
            {
                size_t delimiter = value.rfind('|');
                if (delimiter == std::string::npos)
                    return nullptr;

                std::uint32_t pluginFormID = std::strtol(value.substr(delimiter + 1).c_str(), nullptr, 16);

                if (pluginFormID == 0x0 || pluginFormID > 0xFFFFFF)
                    return nullptr;

                std::string plugin = value.substr(0, delimiter);

#ifdef OBSEAPI
                RE::TESDataHandler* data = RE::TESDataHandler::GetSingleton();
                assert(data);

                for (auto file : data->listFiles)
                {
                    if (file->GetFilename() == plugin)
                    {
                        if (file->IsActive())
                            formID = (file->GetCompileIndex() << 24) | pluginFormID;

                        break;
                    }
                }
#endif
            }
        }
        else
        {
            return nullptr;
        }

        if (formID == 0x0)
            return nullptr;

        FormType* form = nullptr;

#ifdef OBSEAPI
        form = RE::TESForm::LookupByID<FormType>(formID);
#endif

        return form;
    }

    template<typename FormType>
    static std::vector<FormType*> internal_get_form_array(toml_node_view node)
    {
        std::vector<FormType*> result{};

        if (node.is_array())
        {
            auto array = node.as_array();
            for (auto it = array->begin(); it != array->end(); ++it)
            {
                FormType* form = internal_get_form<FormType>(toml_node_view{ *it });
                if (form)
                    result.push_back(form);
            }
        }
        else
        {
            FormType* single = internal_get_form<FormType>(node);
            if (single)
                result.push_back(single);
        }

        return result;
    }
};
