#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/property_bag.hxx>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace Er
{

namespace
{

PropertyMap loadObject(auto&& o, std::size_t depth);


PropertyVector loadArray(auto&& a, std::size_t depth)
{
    if (depth < 1)
        ErThrow("JSON is too nested");

    PropertyVector v;

    for (rapidjson::Value::ConstValueIterator it = a.Begin(); it != a.End(); ++it)
    {
        if (it->IsArray())
        {
            v.emplace_back(std::string_view(""), loadArray(it->GetArray(), depth - 1));
        }
        else if (it->IsObject())
        {
            v.emplace_back(std::string_view(""), loadObject(it->GetObject(), depth - 1));
        }
        else if (it->IsBool())
        {
            v.emplace_back(std::string_view(""), it->GetBool());
        }
        else if (it->IsInt())
        {
            // always load ints as Int64
            v.emplace_back(std::string_view(""), std::int64_t(it->GetInt()));
        }
        else if (it->IsInt64())
        {
            v.emplace_back(std::string_view(""), it->GetInt64());
        }
        else if (it->IsUint())
        {
            // always load ints as Int64
            v.emplace_back(std::string_view(""), std::int64_t(it->GetUint()));
        }
        else if (it->IsUint64())
        {
            v.emplace_back(std::string_view(""), std::int64_t(it->GetUint64()));
        }
        else if (it->IsDouble())
        {
            v.emplace_back(std::string_view(""), it->GetDouble());
        }
        else if (it->IsString())
        {
            v.emplace_back(std::string_view(""), std::string_view(it->GetString(), it->GetStringLength()));
        }
    }

    return v;
}

PropertyMap loadObject(auto&& o, std::size_t depth)
{
    if (depth < 1)
        ErThrow("JSON is too nested");

    PropertyMap m;

    for (rapidjson::Value::ConstMemberIterator it = o.MemberBegin(); it != o.MemberEnd(); ++it)
    {
        auto name = std::string_view(it->name.GetString(), it->name.GetStringLength());
        auto& value = it->value;

        if (value.IsArray())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, loadArray(value.GetArray(), depth - 1)));
        }
        else if (value.IsObject())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, loadObject(value.GetObject(), depth - 1)));
        }
        else if (value.IsBool())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, value.GetBool()));
        }
        else if (value.IsInt())
        {
            // always load ints as Int64
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, std::int64_t(value.GetInt())));
        }
        else if (value.IsInt64())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, value.GetInt64()));
        }
        else if (value.IsUint())
        {
            // always load ints as Int64
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, std::int64_t(value.GetUint())));
        }
        else if (value.IsUint64())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, std::int64_t(value.GetUint64())));
        }
        else if (value.IsDouble())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, value.GetDouble()));
        }
        else if (value.IsString())
        {
            m.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, std::string_view(value.GetString(), value.GetStringLength())));
        }
    }

    return m;
}

} // namespace {}


ER_RTL_EXPORT Property loadJson(std::string_view json, std::size_t depth)
{
    rapidjson::Document doc;
    doc.Parse(json.data(), json.length());
    if (doc.HasParseError())
    {
        auto err = doc.GetParseError();
        std::string text = rapidjson::GetParseError_En(err);
        ErThrow(Er::format("Invalid JSON at offset {}: {}", doc.GetErrorOffset(), std::move(text)));
    }

    if (doc.IsArray())
    {
        return Property("", loadArray(doc.GetArray(), depth));
    }
    else if (doc.IsObject())
    {
        return Property("", loadObject(doc.GetObject(), depth));
    }

    return {};
}

} // namespace Er {}