#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/rtl/util/string_util.hxx>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#undef GetObject

namespace Er
{

namespace
{

PropertyMap loadObject(auto&& o, std::size_t depth);


PropertyVector loadArray(auto&& a, std::size_t depth)
{
    if (depth < 1)
        throw Exception(std::source_location::current(), Error{ Result::InvalidInput, GenericError }, Exception::Message("JSON is too nested"));

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
        throw Exception(std::source_location::current(), Error{ Result::InvalidInput, GenericError }, Exception::Message("JSON is too nested"));

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
        throw Exception(
            std::source_location::current(), 
            Error{ Result::InvalidInput, GenericError },
            Exception::Message(Er::format("Invalid JSON at offset {}: {}", doc.GetErrorOffset(), std::move(text)))
        );
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


namespace
{

struct FindByPath
{
    constexpr FindByPath(const PropertyMap& bag) noexcept
        : currentMap(&bag)
    {
    }

    constexpr FindByPath(const PropertyVector& bag) noexcept
        : currentVector(&bag)
    {
    }

    CallbackResult operator()(char const* start, std::size_t length) noexcept
    {
        const Property* current = nullptr;

        if (currentMap)
        {
            current = findProperty(*currentMap, std::string_view(start, length));
        }
        else if (currentVector)
        {
            current = findProperty(*currentVector, std::string_view(start, length));
        }
        else
        {
            // this means the search path is not yet ended, 
            // but the current item is neither a map nor vector
            // and thus we have nowhere to go
            found = nullptr;
            return CallbackResult::Cancel;
        }

        if (!current)
            return CallbackResult::Cancel;

        // we don't know yet whether this is the end of path
        // just save the current results and continue
        found = current;

        if (current->type() == Property::Type::Map)
        {
            currentMap = current->getMap();
            currentVector = nullptr;
        }
        else if (current->type() == Property::Type::Vector)
        {
            currentVector = current->getVector();
            currentMap = nullptr;
        }
        else
        {
            currentVector = nullptr;
            currentMap = nullptr;
        }

        return CallbackResult::Continue;
    }
    
    const PropertyMap* currentMap = nullptr;
    const PropertyVector* currentVector = nullptr;
    const Property* found = nullptr;
};


} // namespace {}


ER_RTL_EXPORT const Property* findPropertyByPath(const PropertyMap& bag, std::string_view path, char separator, std::optional<Property::Type> type) noexcept
{
    FindByPath fbp(bag);

    Util::split(path, separator, fbp);

    // check type
    if (fbp.found)
    {
        if (type && fbp.found->type() != *type)
            return nullptr;
    }

    return fbp.found;
}

ER_RTL_EXPORT const Property* findPropertyByPath(const PropertyVector& bag, std::string_view path, char separator, std::optional<Property::Type> type) noexcept
{
    FindByPath fbp(bag);

    Util::split(path, separator, fbp);

    // check type
    if (fbp.found)
    {
        if (type && fbp.found->type() != *type)
            return nullptr;
    }

    return fbp.found;
}


} // namespace Er {}