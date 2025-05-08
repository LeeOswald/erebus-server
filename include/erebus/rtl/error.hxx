#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/rtl/string_literal.hxx>

#include <system_error>


namespace Er
{

using ResultCode = std::int32_t;

namespace Result
{

enum : std::int32_t
{
    Ok = 0,
    OutOfMemory,
    Internal,
    ScriptError,
    InvalidInput,
    BadSymlink,
    BadPlugin,
    BadConfiguration,
    FailedPrecondition,
};


} // namespace Result {}


struct IErrorCategory;

/**
* Error code and category that (unlike std::error_code) can be marshaled through RPC
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

    Error(std::error_code ec);

    constexpr Code code() const noexcept
    {
        return m_code;
    }

    IErrorCategory const* category() const noexcept
    {
        return m_category;
    }
    
    constexpr operator bool() const noexcept
    {
        return (m_code != Success);
    }

    std::string message() const;

protected:
    Code m_code = Success;
    IErrorCategory const* m_category = nullptr;
    
};


struct IErrorCategory
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IErrorCategory";

    virtual bool local() const noexcept = 0;
    virtual std::string_view name() const noexcept = 0;
    virtual std::string message(Error::Code code) const = 0;

protected:
    ~IErrorCategory() = default;
};


extern ER_RTL_EXPORT IErrorCategory const* const GenericError;

extern ER_RTL_EXPORT IErrorCategory const* const PosixError;

#if ER_WINDOWS
extern ER_RTL_EXPORT IErrorCategory const* const Win32Error;
#endif


ER_RTL_EXPORT void registerErrorCategory(std::string_view name, IErrorCategory* cat);
ER_RTL_EXPORT void unregisterErrorCategory(IErrorCategory* cat) noexcept;
ER_RTL_EXPORT IErrorCategory const* lookupErrorCategory(std::string_view name) noexcept;


} // namespace Er {}