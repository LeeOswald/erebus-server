#include "config.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>


namespace Er
{

namespace Private
{

ServerConfig loadConfig(const std::string& path)
{
    auto contents = Er::Util::loadFile(path);
    
    rapidjson::Document doc;
    doc.ParseInsitu(contents.data());
    if (doc.HasParseError())
    {
        auto err = doc.GetParseError();
        throw Er::Exception(ER_HERE(), Util::format("Failed to parse config: [%s] at %zu", rapidjson::GetParseError_En(err), doc.GetErrorOffset()));
    }

    if (!doc.IsObject())
        throw Er::Exception(ER_HERE(), "Config is not a JSON object");

    ServerConfig cfg;

    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m)
    {
        auto name = m->name.GetString();
        if (!std::strcmp(name, "verbose"))
        {
            if (!m->value.IsInt())
                throw Er::Exception(ER_HERE(), "Config.verbose is not an int");
            cfg.verbose = m->value.GetInt();
        }
        else if (!std::strcmp(name, "logfile"))
        {
            if (!m->value.IsString())
                throw Er::Exception(ER_HERE(), "Config.logfile is not a string");
            cfg.logfile = m->value.GetString();
        }
        else if (!std::strcmp(name, "certificate"))
        {
            if (!m->value.IsString())
                throw Er::Exception(ER_HERE(), "Config.certificate is not a string");
            cfg.certificate = m->value.GetString();
        }
        else if (!std::strcmp(name, "key"))
        {
            if (!m->value.IsString())
                throw Er::Exception(ER_HERE(), "Config.key is not a string");
            cfg.privateKey = m->value.GetString();
        }
        else if (!std::strcmp(name, "root"))
        {
            if (!m->value.IsString())
                throw Er::Exception(ER_HERE(), "Config.root is not a string");
            cfg.rootCA = m->value.GetString();
        }
        else if (!std::strcmp(name, "userdb"))
        {
            if (!m->value.IsString())
                throw Er::Exception(ER_HERE(), "Config.userdb is not a string");
            cfg.userDb = m->value.GetString();
        }
        else if (!std::strcmp(name, "endpoints"))
        {
            if (!m->value.IsArray())
                throw Er::Exception(ER_HERE(), "Config.endpoints is not an array");
            
            for (size_t index = 0; index < m->value.Size(); ++index)
            {
                auto& entry = m->value[index];
                if (!entry.IsObject())
                    throw Er::Exception(ER_HERE(), Util::format("Config.endpoints[%zu] is not an object", index));

                ServerConfig::Endpoint ep;
                for (auto m = entry.MemberBegin(); m != entry.MemberEnd(); ++m)
                {
                    auto name = m->name.GetString();
                    if (!std::strcmp(name, "endpoint"))
                    {
                        if (!m->value.IsString())
                            throw Er::Exception(ER_HERE(), Util::format("Config.endpoints[%zu].endpoint is not a string", index));
                        ep.endpoint = m->value.GetString();
                    }
                    else if (!std::strcmp(name, "ssl"))
                    {
                        if (!m->value.IsBool())
                            throw Er::Exception(ER_HERE(), Util::format("Config.endpoints[%zu].ssl is not a bool", index));
                        ep.ssl = m->value.GetBool();
                    }
                }

                cfg.endpoints.push_back(std::move(ep));
            }
        }
        else if (!std::strcmp(name, "plugins"))
        {
            if (!m->value.IsArray())
                throw Er::Exception(ER_HERE(), "Config.plugins is not an array");
            
            for (size_t index = 0; index < m->value.Size(); ++index)
            {
                auto& entry = m->value[index];
                if (!entry.IsString())
                    throw Er::Exception(ER_HERE(), Util::format("Config.plugins[%zu] is not a string", index));

                auto path = entry.GetString();
                cfg.plugins.push_back(path);
            }
        }
    }

    return cfg;
}

} // namespace Private {}

} // namespace Er {}