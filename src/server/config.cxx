#include "config.hxx"

#include <erebus/rtl/exception.hxx>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <rapidjson/error/en.h>

#include <sstream>


namespace
{

const std::string_view kSchema =
R"(
{
    "comment": "JSON Schema for configuration",
    "type": "object",
    "properties": {
        "log_file": {
            "type": "string"
        },
        "keep_logs": {
            "type": "number"
        },
        "max_log_size": {
            "type": "number"
        },
        "pid_file": {
            "type": "string"
        },
        "endpoints": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "endpoint": {
                        "type": "string"
                    },
                    "ssl": {
                        "type": "boolean"
                    },
                    "certificate": {
                        "type": "string"
                    },
                    "private_key": {
                        "type": "string"
                    },
                    "root_ca": {
                        "type": "string"
                    }
                },
                "required": ["endpoint"]
            }
        },
        "plugins": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string"
                    },
                    "args": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type":"string"
                                },
                                "value": {
                                    "type":"string"
                                }
                            }
                        }
                    },
                    "enabled": {
                        "type": "boolean"
                    }
                },
                "required": ["path"]
            }
        }
    },
    "required": ["log_file", "endpoints", "plugins", "pid_file"]
}
)";

rapidjson::Document loadAndValidate(const std::string& path)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(kSchema.data(), kSchema.length());
    ErAssert(!schemaDocument.HasParseError());

    valijson::Schema schema;
    valijson::SchemaParser parser;
    valijson::adapters::RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    parser.populateSchema(schemaDocumentAdapter, schema);
    
    rapidjson::Document targetDocument;
    if (!valijson::utils::loadDocument(path, targetDocument)) 
        ErThrow(Er::format("Failed to load config from [{}]", path));

    valijson::Validator validator(valijson::Validator::kStrongTypes);
    valijson::ValidationResults results;
    valijson::adapters::RapidJsonAdapter targetDocumentAdapter(targetDocument);
    if (!validator.validate(schema, targetDocumentAdapter, &results)) 
    {
        std::ostringstream ss;
        
        valijson::ValidationResults::Error error;
        unsigned int errorNum = 1;
        while (results.popError(error)) 
        {
            std::string context;
            for (auto itr = error.context.begin(); itr != error.context.end(); itr++) 
            {
                context += *itr;
            }

            if (errorNum > 1)
                ss << "; ";

            ss << "Error #" << errorNum << ": [";
            ss << "context: " << context << ", ";
            ss << "description: " << error.description << "]";
            
            ++errorNum;
        }
        
        ErThrow(Er::format("Failed to validate config: {}", ss.str()));
    }

    return targetDocument;
}

} // namespace {}


ServerConfig loadConfig(const std::string& path)
{
    auto doc = loadAndValidate(path);

    ServerConfig cfg;

    for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m)
    {
        auto name = m->name.GetString();
        if (!std::strcmp(name, "log_file"))
            cfg.logFile = m->value.GetString();
        else if (!std::strcmp(name, "keep_logs"))
            cfg.keepLogs = m->value.GetInt();
        else if (!std::strcmp(name, "max_log_size"))
            cfg.maxLogSize = m->value.GetUint64() * 1024 * 1024;
        else if (!std::strcmp(name, "pid_file"))
            cfg.pidFile = m->value.GetString();
        else if (!std::strcmp(name, "endpoints"))
        {
            for (size_t index = 0; index < m->value.Size(); ++index)
            {
                auto& entry = m->value[index];

                ServerConfig::Endpoint ep;
                for (auto m = entry.MemberBegin(); m != entry.MemberEnd(); ++m)
                {
                    auto name = m->name.GetString();
                    if (!std::strcmp(name, "endpoint"))
                        ep.endpoint = m->value.GetString();
                    else if (!std::strcmp(name, "ssl"))
                        ep.useTls = m->value.GetBool();
                    else if (!std::strcmp(name, "certificate"))
                        ep.certificate = m->value.GetString();
                    else if (!std::strcmp(name, "private_key"))
                        ep.privateKey = m->value.GetString();
                    else if (!std::strcmp(name, "root_ca"))
                        ep.rootCertificates = m->value.GetString();
                }

                cfg.endpoints.push_back(std::move(ep));
            }
        }
        else if (!std::strcmp(name, "plugins"))
        {
            auto plugins = m->value.GetArray();
            
            for (size_t index = 0; index < plugins.Size(); ++index)
            {
                ServerConfig::Plugin plugin;

                auto& entry = plugins[index];
                for (auto m = entry.MemberBegin(); m != entry.MemberEnd(); ++m)
                {
                    auto name = m->name.GetString();
                    if (!std::strcmp(name, "path"))
                    {
                        plugin.path = m->value.GetString();
                    }
                    else if (!std::strcmp(name, "enabled"))
                    {
                        plugin.enabled = m->value.GetBool();
                    }
                    else if (!std::strcmp(name, "args"))
                    {
                        auto args = m->value.GetArray();
                        for (size_t index = 0; index < m->value.Size(); ++index)
                        {
                            std::string key;
                            std::string val;

                            auto& arg = args[index];
                            for (auto m = arg.MemberBegin(); m != arg.MemberEnd(); ++m)
                            {
                                auto name = m->name.GetString();
                                if (!std::strcmp(name, "name"))
                                    key = m->value.GetString();
                                else if (!std::strcmp(name, "value"))
                                    val = m->value.GetString();

                            }

                            if (!key.empty())
                            {
                                plugin.args.v.emplace_back(std::move(key), std::move(val));
                            }
                        }
                    }
                }

                cfg.plugins.push_back(std::move(plugin));
            }
        }
    }

    return cfg;
}
