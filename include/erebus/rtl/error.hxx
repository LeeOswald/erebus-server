#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/rtl/string_literal.hxx>

namespace Er
{

struct IErrorCategory;

/**
* Error info that carries arbitrary properties and can be marshaled through RPC 
*/

struct ER_RTL_EXPORT Error
{
    using Code = std::int32_t;
    static constexpr Code Success = 0;

    constexpr Error() noexcept = default;

    constexpr Error(Code code, IErrorCategory const* category) noexcept
        : m_code(code)
        , m_category(category)
    {
        ErAssert(!!category || (code == Success));
    }

    Error(const Error&) = default;
    Error& operator=(const Error&) = default;

    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;

    template <typename... _Properties>
        requires (std::is_base_of_v<Property, std::remove_cvref_t<_Properties>> && ...)
    Error(Code code, IErrorCategory const* category, _Properties&&... props)
        : Error(code, category)
    {
        (add(std::forward<_Properties>(props)), ...);
    }

    template <typename _PropertyBag>
        requires std::is_constructible_v<PropertyBag, _PropertyBag>
    Error(Code code, IErrorCategory const* category, _PropertyBag&& props)
        : Error(code, category)
    {
        m_properties = std::forward<_PropertyBag>(props);
    }

    constexpr Code code() const noexcept
    {
        return m_code;
    }

    IErrorCategory const* category() const noexcept
    {
        return m_category;
    }

    PropertyBag const& properties() const noexcept
    {
        return m_properties;
    }

    PropertyBag& properties() noexcept
    {
        return m_properties;
    }

    constexpr operator bool() const noexcept
    {
        return (m_code != Success);
    }

    template <typename _Property>
        requires std::is_base_of_v<Property, std::remove_cvref_t<_Property>>
    Error& add(_Property&& prop)
    {
        this->m_properties.push_back(std::forward<_Property>(prop));
        return *this;
    }

    bool decode();
    
private:
    Code m_code = Success;
    IErrorCategory const* m_category = nullptr;
    PropertyBag m_properties;
};


struct IErrorCategory
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IErrorCategory";

    virtual std::string_view name() const noexcept = 0;
    virtual std::string message(Error::Code code) const = 0;
};


extern ER_RTL_EXPORT IErrorCategory const* const PosixError;

#if ER_WINDOWS
extern ER_RTL_EXPORT IErrorCategory const* const Win32Error;
#endif


ER_RTL_EXPORT void registerErrorCategory(std::string_view name, IErrorCategory* cat);
ER_RTL_EXPORT void unregisterErrorCategory(IErrorCategory* cat) noexcept;
ER_RTL_EXPORT IErrorCategory const* lookupErrorCategory(std::string_view name) noexcept;


namespace ErrorProperties
{

template <typename _DataType, Property::Type _TypeId, StringLiteral _Name>
struct ErrorProperty
    : public Property
{
    using DataType = _DataType;
    static constexpr Property::Type TypeId = _TypeId;
    static constexpr std::string_view Name{ _Name.data(), _Name.size() };

    template <typename _Ty>
        requires std::is_constructible_v<_DataType, _Ty>
    explicit ErrorProperty(_Ty&& v)
        : Property(Name, std::forward<_Ty>(v))
    {
    }
};


//  a brief message provided while issuing the error, e.g., throw Exception(..."Failed to create log");
using Brief = ErrorProperty<std::string, Property::Type::String, "brief">;

// textual description from the error code, e.g., obtained from strerror() of FormatMessage()
using Message = ErrorProperty<std::string, Property::Type::String, "message">;


} // namespace ErrorProperties {}


ER_RTL_EXPORT void logError(Log::ILogger* log, Log::Level level, const Error& e);


} // namespace Er {}