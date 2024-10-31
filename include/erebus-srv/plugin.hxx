#pragma once

#include <erebus/idisposable.hxx>
#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <vector>

namespace Er::Server
{

struct PluginParams
{
    struct Arg
    {
        std::string name;
        std::string value;

        template <typename KeyT, typename ValueT>
        Arg(KeyT&& key, ValueT&& val)
            : name(std::forward<KeyT>(key))
            , value(std::forward<ValueT>(val))
        {}
    };

    Er::Log::ILog* log = nullptr;
    IServer* container = nullptr;
    std::string binary;
    std::vector<Arg> args;
};


struct IPlugin
    : public Er::IDisposable
{
    struct Info
    {
        struct Version
        {
            unsigned major;
            unsigned minor;
            unsigned patch;

            Version(unsigned major, unsigned minor, unsigned patch)
                : major(major)
                , minor(minor)
                , patch(patch)
            {}
        };

        std::string name;
        std::string description;
        Version version;

        Info(std::string_view name, std::string_view description, const Version& version)
            : name(name)
            , description(description)
            , version(version)
        {}
    };

    virtual Info info() const = 0;

protected:
    virtual ~IPlugin() {}
};


typedef IPlugin* (createPlugin)(const PluginParams&);



} // namespace Er::Server {}    