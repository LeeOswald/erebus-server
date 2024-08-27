#pragma once

#include <erebus/exception.hxx>

namespace Er::Lua 
{

class EREBUS_EXPORT LuaException 
    : public Er::Exception
{
public:
    LuaException() = default;

    template <typename MessageT>
    explicit LuaException(Location&& location, MessageT&& message)
        : Er::Exception(std::move(location), std::forward<MessageT>(message))
    {}
};

class EREBUS_EXPORT TypeError
    : public LuaException 
{
public:
    explicit TypeError(Location&& location, std::string expected)
      : LuaException(std::move(location), std::move(expected) + " expected, got no object")
    {}

    explicit TypeError(Location&& location, std::string expected, std::string const& actual)
      : LuaException(std::move(location), std::move(expected) + " expected, got " + actual)
    {}
};

class EREBUS_EXPORT CopyUnregisteredType
    : public LuaException 
{
public:
    using TypeID = std::reference_wrapper<const std::type_info>;
    
    explicit CopyUnregisteredType(Location&& location, TypeID type)
        : LuaException(std::move(location), "Tried to copy an object of an unregistered type")
        , _type(type) 
    {}

    TypeID type() const
    {
        return _type;
    }

private:
    TypeID _type;
};

} // namespace Er::Lua {}
