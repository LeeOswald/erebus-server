#pragma once

#include <erebus/rtl/binary.hxx>
#include <erebus/rtl/bool.hxx>

#include <functional>
#include <vector>


namespace Er
{

enum class PropertyType : uint32_t
{
    // order is important
    Empty,
    Bool,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Double,
    String,
    Binary,
    Max // should go last
};

static_assert(unsigned(PropertyType::Max) <= 0x10, "Align PropertyInfo so that we can fit enough PropertyTypes into it's lower address bits");


struct Property;
struct PropertyInfo;

} // namespace Er {]

namespace Erp
{

ER_RTL_EXPORT std::uint32_t registerPersistentProperty(const Er::PropertyInfo* info);
ER_RTL_EXPORT std::string formatProperty(const Er::PropertyInfo* info, const Er::Property& prop);
ER_RTL_EXPORT const Er::PropertyInfo* allocateTransientProperty(Er::PropertyType type, const std::string& name, const std::string& readableName);

} // namespace Erp {}

namespace Er
{

ER_RTL_EXPORT const PropertyInfo* lookupProperty(const std::string& name) noexcept;
ER_RTL_EXPORT std::uint32_t enumerateProperties(std::function<bool(const PropertyInfo*)> cb) noexcept;


//
// Yes, we DO need that large alignment value
// struct Property carries a pointer to PropertyInfo instance
// and 4 lower bits of this (aligned) pointer are used for PropertyType
//
struct alignas(16) PropertyInfo 
{
    using Formatter = std::function<std::string(const Property&)>;

    using Unique = std::uint32_t;
    static constexpr Unique InvalidUnique = Unique(-1);

    constexpr void* self()
    {
        return this;
    }

    constexpr Unique unique() const noexcept
    {
        return m_unique;
    }

    constexpr PropertyType type() const noexcept
    {
        return m_type;
    }

    constexpr const std::string& name() const noexcept 
    {
        return m_name;
    }

    constexpr const std::string& readableName() const noexcept
    {
        return m_readableName;
    }

    constexpr const Formatter& formatter() const noexcept
    {
        return m_formatter;
    }
    
    ~PropertyInfo() = default;
    
    PropertyInfo(PropertyType type, std::string_view name, std::string_view readableName, Formatter&& formatter = Formatter{})
        : m_type(type)
        , m_name(name)
        , m_readableName(readableName)
        , m_formatter(std::move(formatter))
        , m_unique(Erp::registerPersistentProperty(this))
    {
    }

    struct Transient {};

    PropertyInfo(Transient tag, Unique id, PropertyType type, std::string_view name, std::string_view readableName)
        : m_type(type)
        , m_name(name)
        , m_readableName(readableName)
        , m_formatter()
        , m_unique(id)
    {
    }

    std::string format(const Property& prop) const
    {
        return Erp::formatProperty(this, prop);
    }

private:
    PropertyType m_type;
    std::string m_name;
    std::string m_readableName;
    Formatter m_formatter;
    Unique m_unique;
};


namespace Unspecified
{

extern ER_RTL_EXPORT const PropertyInfo Empty;
extern ER_RTL_EXPORT const PropertyInfo Bool;
extern ER_RTL_EXPORT const PropertyInfo Int32;
extern ER_RTL_EXPORT const PropertyInfo UInt32;
extern ER_RTL_EXPORT const PropertyInfo Int64;
extern ER_RTL_EXPORT const PropertyInfo UInt64;
extern ER_RTL_EXPORT const PropertyInfo Double;
extern ER_RTL_EXPORT const PropertyInfo String;
extern ER_RTL_EXPORT const PropertyInfo Binary;


} // namespace Unspecified {}

    
} // namespace Er {}
