#include "config.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <rapidjson/error/en.h>

#include <sstream>

namespace Er
{

namespace Private
{

namespace
{

static const std::string_view kSchema =
R"(
{
    "comment": "JSON Schema for configuration",
    "type": "object",
    "properties": {
        "verbose": {
            "type": "number",
            "minimum": 0,
            "maximum": 1
        },
        "logfile": {
            "type": "string"
        },
        "certificate": {
            "type": "string"
        },
        "key": {
            "type": "string"
        },
        "root": {
            "type": "string"
        },
        "userdb": {
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
                    }
                },
                "required": ["endpoint"]
            }
        },
        "plugins": {
            "type": "array",
            "items": {
                "type": "string"
            }
        }
    },
    "required": ["logfile", "userdb", "endpoints", "plugins"]
}
)";

rapidjson::Document loadAndValidate(const std::string& path)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(kSchema.data(), kSchema.length());
    assert(!schemaDocument.HasParseError());

    valijson::Schema schema;
    valijson::SchemaParser parser;
    valijson::adapters::RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    parser.populateSchema(schemaDocumentAdapter, schema);
    
    rapidjson::Document targetDocument;
    if (!valijson::utils::loadDocument(path, targetDocument)) 
        throw Er::Exception(ER_HERE(), Util::format("Failed to load config from [%s]", path.c_str()));

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
        
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to validate config: %s", ss.str().c_str()));
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
        if (!std::strcmp(name, "verbose"))
            cfg.verbose = m->value.GetInt();
        else if (!std::strcmp(name, "logfile"))
            cfg.logfile = m->value.GetString();
        else if (!std::strcmp(name, "certificate"))
            cfg.certificate = m->value.GetString();
        else if (!std::strcmp(name, "key"))
            cfg.privateKey = m->value.GetString();
        else if (!std::strcmp(name, "root"))
            cfg.rootCA = m->value.GetString();
        else if (!std::strcmp(name, "userdb"))
            cfg.userDb = m->value.GetString();
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
                        ep.ssl = m->value.GetBool();
                }

                cfg.endpoints.push_back(std::move(ep));
            }
        }
        else if (!std::strcmp(name, "plugins"))
        {
            for (size_t index = 0; index < m->value.Size(); ++index)
            {
                auto& entry = m->value[index];
                auto path = entry.GetString();
                cfg.plugins.push_back(path);
            }
        }
    }

    return cfg;
}

} // namespace Private {}

} // namespace Er {}