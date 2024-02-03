#include "users.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>

namespace Er
{

namespace Private
{

UserDb::~UserDb()
{
}

UserDb::UserDb(const std::string& path)
    : m_path(path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return;

    std::stringstream ss;
    ss << f.rdbuf();
    auto buffer = ss.str();

    rapidjson::Document doc;
    doc.ParseInsitu(buffer.data());
    if (doc.HasParseError())
    {
        auto err = doc.GetParseError();
        throw Er::Exception(ER_HERE(), Util::format("Failed to parse user DB: [%s] at %zu", rapidjson::GetParseError_En(err), doc.GetErrorOffset()));
    }

    if (!doc.IsArray())
        throw Er::Exception(ER_HERE(), "User DB is not a JSON array");

    for (size_t index = 0; index < doc.Size(); ++index)
    {
        auto& entry = doc[index];
        if (!entry.IsObject())
            throw Er::Exception(ER_HERE(), Util::format("User DB entry #%zu is not an object", index));

        // user name
        if (!entry.HasMember("name"))
            throw Er::Exception(ER_HERE(), Util::format("User DB entry #%zu contains no user name", index));

        auto& name = entry["name"];
        if (!name.IsString())
            throw Er::Exception(ER_HERE(), Util::format("User DB entry #%zu contains no valid user name", index));

        std::string strName(name.GetString(), name.GetStringLength());
        auto existing = m_users.find(strName);
        if (existing != m_users.end())
            throw Er::Exception(ER_HERE(), Util::format("User DB contains a duplicate user [%s]", strName.c_str()));

        // salt
        if (!entry.HasMember("salt"))
            throw Er::Exception(ER_HERE(), Util::format("User DB contains no salt for user [%s]", strName.c_str()));

        auto& salt = entry["salt"];
        if (!salt.IsString())
            throw Er::Exception(ER_HERE(), Util::format("User DB contains no valid salt for user [%s]", strName.c_str()));

        std::string strSalt(salt.GetString(), salt.GetStringLength());

        // password hash
        if (!entry.HasMember("hash"))
            throw Er::Exception(ER_HERE(), Util::format("User DB contains no hash for user [%s]", strName.c_str()));

        auto& hash = entry["hash"];
        if (!hash.IsString())
            throw Er::Exception(ER_HERE(), Util::format("User DB contains no valid hash for user [%s]", strName.c_str()));

        std::string strHash(hash.GetString(), hash.GetStringLength());

        m_users.insert({ strName, Server::User(strName, strSalt, strHash) });
    }
}

void UserDb::add(const Server::User& u)
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

std::vector<Server::User> UserDb::enumerate() const
{
    std::vector<Server::User> result;

    std::lock_guard l(m_mutex);

    result.reserve(m_users.size());
    for (auto& u : m_users)
    {
        result.push_back(u.second);
    }

    return result;
}

std::optional<Server::User> UserDb::lookup(const std::string& name) const
{
    std::lock_guard l(m_mutex);

    auto it = m_users.find(name);
    if (it == m_users.end())
        return std::nullopt;

    return it->second;
}

void UserDb::save()
{
    if (!m_dirty)
        return;

    std::ofstream f(m_path, std::ios::trunc);
    if (!f.is_open())
        throw Er::Exception(ER_HERE(), Util::format("Failed to overwrite user DB file [%s]", m_path.c_str()));

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartArray();

    std::lock_guard l(m_mutex);

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