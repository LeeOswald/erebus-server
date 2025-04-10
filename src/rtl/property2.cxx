#include <erebus/rtl/property2.hxx>

namespace Er
{

std::string_view Property2::typeToString(Type type) noexcept
{
    static constexpr std::string_view names[] =
    {
        "Empty",
        "Bool",
        "Int32",
        "UInt32",
        "Int64",
        "UInt64",
        "Double",
        "String",
        "Binary",
        "Map"
    };

    if (static_cast<std::size_t>(type) >= _countof(names))
        return "<invalid type>";

    return names[static_cast<std::size_t>(type)];
}

bool Property2::_eq(const Property2& other) const noexcept
{
    using EqFn = bool (Property2::*)(const Property2&) const noexcept;

    static EqFn s_eqFns[] =
    {
        &Property2::_eqEmpty,
        &Property2::_eqBool,
        &Property2::_eqInt32,
        &Property2::_eqUInt32,
        &Property2::_eqInt64,
        &Property2::_eqUInt64,
        &Property2::_eqDouble,
        &Property2::_eqString,
        &Property2::_eqBinary,
        &Property2::_eqMap
    };

    auto ty = type();
    if (ty != other.type())
        return false;

    auto idx = static_cast<std::size_t>(ty);
    ErAssert(idx < _countof(s_eqFns));
    return std::invoke(s_eqFns[idx], *this, other);
}

bool Property2::_eqEmpty(const Property2& other) const noexcept
{
    return true;
}

bool Property2::_eqBool(const Property2& other) const noexcept
{
    auto v1 = getBool();
    auto v2 = other.getBool();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqInt32(const Property2& other) const noexcept
{
    auto v1 = getInt32();
    auto v2 = other.getInt32();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqUInt32(const Property2& other) const noexcept
{
    auto v1 = getUInt32();
    auto v2 = other.getUInt32();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqInt64(const Property2& other) const noexcept
{
    auto v1 = getInt64();
    auto v2 = other.getInt64();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqUInt64(const Property2& other) const noexcept
{
    auto v1 = getUInt64();
    auto v2 = other.getUInt64();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqDouble(const Property2& other) const noexcept
{
    auto v1 = getDouble();
    auto v2 = other.getDouble();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqString(const Property2& other) const noexcept
{
    auto v1 = getString();
    auto v2 = other.getString();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqBinary(const Property2& other) const noexcept
{
    auto v1 = getBinary();
    auto v2 = other.getBinary();
    ErAssert(!!v1 && !!v2);
    return *v1 == *v2;
}

bool Property2::_eqMap(const Property2& other) const noexcept
{
    auto v1 = getMap();
    auto v2 = other.getMap();
    ErAssert(!!v1 && !!v2);

    if (v1->size() != v2->size())
        return false;

    auto it1 = v1->begin();
    auto it2 = v2->begin();

    while ((it1 != v1->end()) && (it2 != v2->end()))
    {
        if (it1->first != it2->first)
            return false;

        if (it1->second != it2->second)
            return false;

        ++it1;
        ++it2;
    }

    return true;
}

std::string Property2::_str() const
{
    using StrFn = std::string(Property2::*)() const;

    static StrFn s_strFns[] =
    {
        &Property2::_strEmpty,
        &Property2::_strBool,
        &Property2::_strInt32,
        &Property2::_strUInt32,
        &Property2::_strInt64,
        &Property2::_strUInt64,
        &Property2::_strDouble,
        &Property2::_strString,
        &Property2::_strBinary,
        &Property2::_strMap
    };

    auto ty = type();
    auto idx = static_cast<std::size_t>(ty);
    ErAssert(idx < _countof(s_strFns));
    return std::invoke(s_strFns[idx], *this);
}

std::string Property2::_strEmpty() const
{
    return "[empty]";
}

std::string Property2::_strBool() const
{
    auto v = getBool();
    ErAssert(v);
    return *v ? "True" : "False";
}

std::string Property2::_strInt32() const
{
    auto v = getInt32();
    ErAssert(v);
    return std::to_string(*v);
}

std::string Property2::_strUInt32() const
{
    auto v = getUInt32();
    ErAssert(v);
    return std::to_string(*v);
}

std::string Property2::_strInt64() const
{
    auto v = getInt64();
    ErAssert(v);
    return std::to_string(*v);
}

std::string Property2::_strUInt64() const
{
    auto v = getUInt64();
    ErAssert(v);
    return std::to_string(*v);
}

std::string Property2::_strDouble() const
{
    auto v = getDouble();
    ErAssert(v);
    return std::to_string(*v);
}

std::string Property2::_strString() const
{
    auto v = getString();
    ErAssert(v);
    return *v;
}

std::string Property2::_strBinary() const
{
    auto v = getBinary();
    ErAssert(v);
    std::ostringstream ss;
    ss << *v;
    return ss.str();
}

std::string Property2::_strMap() const
{
    auto v = getMap();
    ErAssert(v);
    if (v->empty())
        return "[]";

    std::ostringstream ss;

    bool empty = true;
    ss << "[ ";
    for (auto& prop : *v)
    {
        if (empty)
        {
            empty = false;
        }
        else
        {
            ss << ", ";
        }

        ss << "{ \"" << prop.first << "\" = \"" << prop.second.str() << "\" }";
    }

    ss << " ]";

    return ss.str();
}

} // namespace Er {}