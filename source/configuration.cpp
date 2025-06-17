#include "configuration.h"

plugin_configuration::plugin_configuration(const std::string_view& configFilePath) :
    configFilePath_{ configFilePath }
{
#ifdef COMMONLIB_STUFF
    pluginFileName_ = Get("General", "sPluginFileName", Plugin::PLUGINFILE);
#endif
}

toml::table& plugin_configuration::get_table()
{
    if (!table_)
    {
        try
        {
            table_ = new toml::table{ std::move(toml::parse_file(configFilePath_)) };
        }
        catch (const toml::parse_error&)
        {
            // Just create an empty table and save it later if there are any changes.
            table_ = new toml::table{};
        }
    }
    return *table_;
}

toml::table& plugin_configuration::get_subtable(const std::string_view& section)
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
            std::ofstream file{ configFilePath_.data()};
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