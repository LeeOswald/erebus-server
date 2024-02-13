#include "users.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>
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
    "comment": "JSON Schema for user DB",
    "type": "array",
    "items": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string"
            },
            "salt": {
                "type": "string"
            },
            "hash": {
                "type": "string"
            }
        },
        "required": ["name", "salt", "hash"]
    }
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
        throw Er::Exception(ER_HERE(), Util::format("Failed to load user DB from [%s]", path.c_str()));

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

        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to validate user DB: %s", ss.str().c_str()));
    }

    return targetDocument;
}

} // namespace {}


UserDb::~UserDb()
{
}

UserDb::UserDb(const std::string& path)
    : m_path(path)
{
    auto doc = loadAndValidate(path);

    for (size_t index = 0; index < doc.Size(); ++index)
    {
        auto& entry = doc[index];

        // name
        auto& name = entry["name"];
        std::string strName(name.GetString(), name.GetStringLength());
        auto existing = m_users.find(strName);
        if (existing != m_users.end())
            throw Er::Exception(ER_HERE(), Util::format("User DB contains a duplicate user [%s]", strName.c_str()));

        // salt
        auto& salt = entry["salt"];
        std::string strSalt(salt.GetString(), salt.GetStringLength());

        // password hash
        auto& hash = entry["hash"];
        std::string strHash(hash.GetString(), hash.GetStringLength());

        m_users.insert({ strName, Er::Server::Private::User(strName, strSalt, strHash) });
    }
}

void UserDb::add(const Er::Server::Private::User& u)
{
    std::lock_guard l(m_mutex);

    auto it = m_users.find(u.name);
    if (it != m_users.end())
        throw Er::Exception(ER_HERE(), Util::format("User DB already contains user [%s]", u.name.c_str()));

    m_users.insert({ u.name, u });
    m_dirty = true;
}

void UserDb::remove(const std::string& name)
{
    std::lock_guard l(m_mutex);

    auto it = m_users.find(name);
    if (it == m_users.end())
        throw Er::Exception(ER_HERE(), Util::format("User DB contains no user [%s]", name.c_str()));

    m_users.erase(it);
    m_dirty = true;
}

std::vector<Er::Server::Private::User> UserDb::enumerate() const
{
    std::vector<Er::Server::Private::User> result;

    std::shared_lock l(m_mutex);

    result.reserve(m_users.size());
    for (auto& u : m_users)
    {
        result.push_back(u.second);
    }

    return result;
}

std::optional<Er::Server::Private::User> UserDb::lookup(const std::string& name) const
{
    std::shared_lock l(m_mutex);

    auto it = m_users.find(name);
    if (it == m_users.end())
        return std::nullopt;

    return it->second;
}

void UserDb::save()
{
    std::shared_lock l(m_mutex);

    if (!m_dirty)
        return;

    std::ofstream f(m_path, std::ios::trunc);
    if (!f.is_open())
        throw Er::Exception(ER_HERE(), Util::format("Failed to overwrite user DB file [%s]", m_path.c_str()));

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    writer.StartArray();

    for (auto& u : m_users)
    {
        writer.StartObject();

        writer.Key("name");
        writer.String(u.second.name.data(), u.second.name.length());

        writer.Key("salt");
        writer.String(u.second.pwdSalt.data(), u.second.pwdSalt.length());

        writer.Key("hash");
        writer.String(u.second.pwdHash.data(), u.second.pwdHash.length());

        writer.EndObject();
    }

    writer.EndArray();

    std::string_view out(sb.GetString());
    f << out;

    f.close();
    m_dirty = false;
}

} // namespace Private {}

} // namespace Er {}