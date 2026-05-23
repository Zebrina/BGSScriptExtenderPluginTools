#include "configuration.h"

#include "windows_lean_and_mean.h"

#include <format>
#include <queue>

plugin_configuration::plugin_configuration(const std::string_view configFilePath) :
    filePath_{ configFilePath }
{
#ifdef COMMONLIB_STUFF
    pluginFileName_ = Get("General", "sPluginFileName", Plugin::PLUGINFILE);
#endif
}

/*
void plugin_configuration::for_each_configuration(const std::string& folder, void(*action)(plugin_configuration&), std::source_location source_loc)
{
    WIN32_FIND_DATA ffd;
    HANDLE handle = FindFirstFile(folder.c_str(), &ffd);
    if (handle == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && is_toml_file(ffd.cFileName))
        {
            plugin_configuration config{ ffd.cFileName };
            action(config);
        }
    }
    while (FindNextFile(handle, &ffd));

    FindClose(handle);
}

void plugin_configuration::for_each_section(for_each_action(*action)(table_interface&), std::source_location source_loc)
{
    std::queue<table_interface> suspended{};

    for (auto& section : get_table())
    {
        if (!section.second.is_table())
            continue;

        table_interface table{ section.first, section.second.as_table() };
        for_each_action result = action(table);
        if (result == for_each_break)
            break;
        else if (result == for_each_yield)
            suspended.push(table);
    }

    for (; !suspended.empty(); suspended.pop())
    {
        table_interface table = suspended.front();
        for_each_action result = action(table);
        if (result == for_each_break)
            break;
        else if (result == for_each_yield)
            suspended.push(table);
    }
}
*/

toml::table& plugin_configuration::get_table()
{
    if (!table_)
    {
        try
        {
            table_ = new toml::table{ std::move(toml::parse_file(filePath_)) };
        }
        catch (const toml::parse_error&)
        {
            // Just create an empty table and save it later if there are any changes.
            table_ = new toml::table{};
        }
    }
    return *table_;
}

toml::table& plugin_configuration::get_subtable(const std::string_view section)
{
    toml::table& table = get_table();
    table.insert(section, toml::table{});
    return *table.get_as<toml::table>(section);
}

void plugin_configuration::close()
{
    if (table_)
    {
        if (modified_)
        {
            std::ofstream file{ filePath_.data()};
            if (file.is_open())
            {
                file << static_cast<const toml::table&>(*table_);
                file.close();
            }
            else
            {
                //SKSE::log::error("Failed to open config file at '{}' for writing."sv, Plugin::CONFIGFILE);
                throw std::exception();
            }
        }
        delete table_;
        table_ = nullptr;
    }
}

bool plugin_configuration::is_toml_file(const char* file)
{
    if (!file)
        return false;

    static constexpr const char TOML[] = ".toml";

    size_t len = strlen(file);
    if (len <= sizeof(TOML))
        return false;

    return _stricmp(file + len - (sizeof(TOML) - 1), TOML) == 0;
}

plugin_configuration::directory_iterator::data::data(const char* folder) :
    folder{ folder },
    handle{ INVALID_HANDLE_VALUE },
    current{ nullptr }
{ }

plugin_configuration::directory_iterator::data::~data()
{
    FindClose(handle);
    delete current;
}

bool plugin_configuration::directory_iterator::data::find_next()
{
    WIN32_FIND_DATA ffd;

    delete current;
    current = nullptr;

    while (true)
    {
        bool success;

        if (handle == INVALID_HANDLE_VALUE)
        {
            std::string findPath = std::format("{}\\*.toml", folder);
            handle = FindFirstFile(findPath.c_str(), &ffd);
            success = (handle != INVALID_HANDLE_VALUE);
        }
        else
        {
            success = FindNextFile(handle, &ffd);
        }

        if (!success)
            return false;

        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            current = new plugin_configuration(std::format("{}\\{}", folder, ffd.cFileName));
            return true;
        }
    }
}

void plugin_configuration::directory_iterator::destruct()
{
    delete data_;
    data_ = nullptr;
}
