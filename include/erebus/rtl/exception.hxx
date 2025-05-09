#pragma once

#include <erebus/rtl/error.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/property_bag.hxx>

#if ER_ENABLE_STACKTRACE
    #include <boost/stacktrace.hpp>
#endif

#include <stdexcept>
#include <vector>


/**
* exception class with (almost) arbitrary properties
* that can be marshaled through RPC
*/

namespace Er
{

template <typename _DataType, Property::Type _TypeId, StringLiteral _Name>
struct ExceptionProperty
    : public Property
{
    using DataType = _DataType;
    static constexpr Property::Type TypeId = _TypeId;
    static constexpr std::string_view Name{ _Name.data(), _Name.size() };

    template <typename _Ty>
        requires std::is_constructible_v<_DataType, _Ty>
    explicit ExceptionProperty(_Ty&& v, SemanticCode semantics = Semantics::Default)
        : Property(Name, std::forward<_Ty>(v), semantics)
    {
    }
};


namespace ExceptionProperties
{

//  a brief message provided while issuing the error
using Brief = ExceptionProperty<std::string, Property::Type::String, "brief">;

// textual description from the error code, e.g., obtained from strerror() of FormatMessage()
using Message = ExceptionProperty<std::string, Property::Type::String, "message">;

// failed object name, e.g., file name that could not be opened
using ObjectName = ExceptionProperty<std::string, Property::Type::String, "object">;


} // namespace ExceptionProperties {}


class ER_RTL_EXPORT Exception
    : public std::exception
    , public Error
{
#if ER_ENABLE_STACKTRACE
#if ER_DEBUG
    static constexpr std::size_t StackFramesToSkip = 3;
#else
    static constexpr std::size_t StackFramesToSkip = 2;
#endif
    static constexpr std::size_t StackFramesToCapture = 256;
#endif

public:
    template <typename... _Properties>
        requires (std::is_base_of_v<Property, std::remove_cvref_t<_Properties>> && ...)
    Exception(std::source_location location, const Error& e, _Properties&&... props)
        : Error(e.code(), e.category())
        , m_location(location)
#if ER_ENABLE_STACKTRACE
        , m_stack{ StackFramesToSkip, StackFramesToCapture }
#endif
    {
        (add(std::forward<_Properties>(props)), ...);
    }

    template <typename _Brief, typename... _Properties>
        requires std::is_constructible_v<std::string, _Brief> && (std::is_base_of_v<Property, std::remove_cvref_t<_Properties>> && ...)
    Exception(std::source_location location, const Error& e, _Brief&& brief, _Properties&&... props)
        : Error(e.code(), e.category())
        , m_location(location)
#if ER_ENABLE_STACKTRACE
        , m_stack{ StackFramesToSkip, StackFramesToCapture }
#endif
    {
        add(ExceptionProperties::Brief(std::forward<_Brief>(brief)));
        (add(std::forward<_Properties>(props)), ...);
    }

    struct Message
    {
        template <typename _Message>
            requires std::is_constructible_v<std::string, _Message>
        explicit Message(_Message&& message)
            : message(std::forward<_Message>(message))
        {
        }

        std::string message;
    };

    template <typename... _Properties>
        requires (std::is_base_of_v<Property, std::remove_cvref_t<_Properties>> && ...)
    Exception(std::source_location location, const Error& e, Message&& message, _Properties&&... props)
        : Error(e.code(), e.category())
        , m_location(location)
#if ER_ENABLE_STACKTRACE
        , m_stack{ StackFramesToSkip, StackFramesToCapture }
#endif
    {
        add(ExceptionProperties::Message(std::move(message.message)));
        (add(std::forward<_Properties>(props)), ...);
    }

    Exception(const Exception&) = default;
    Exception& operator=(const Exception&) = default;

    Exception(Exception&&) noexcept = default;
    Exception& operator=(Exception&&) noexcept = default;

    char const* what() const noexcept override
    {
        if (m_message.empty())
            m_message = message();

        return m_message.c_str();
    }

    std::string const& message() const noexcept;

    std::source_location location() const noexcept
    {
        return m_location;
    }

#if ER_ENABLE_STACKTRACE
    auto const& stack() const noexcept
    {
        return m_stack;
    }
#endif

    PropertyBag const& properties() const noexcept
    {
        return m_properties;
    }

    PropertyBag& properties() noexcept
    {
        return m_properties;
    }

    template <typename _Property>
        requires std::is_base_of_v<Property, std::remove_cvref_t<_Property>>
    Exception const& add(_Property&& prop) const // 'const' because we want the ability to add props on the fly
    {
        m_properties.push_back(std::forward<_Property>(prop));
        return *this;
    }

    std::string const* decode() const;

protected:
    std::source_location m_location;
#if ER_ENABLE_STACKTRACE
    boost::stacktrace::stacktrace m_stack;
#endif
    mutable std::string m_message;
    mutable PropertyBag m_properties; // yep, 'mutable'; we want the ability to add props on the fly
};


template <typename ExceptionVisitor>
auto dispatchException(const std::exception_ptr& ep, ExceptionVisitor& visitor)
{
    try
    {
        std::rethrow_exception(ep);
    }
    catch (const Exception& e)
    {
        return visitor(e);
    }
    catch (const std::bad_alloc& e)
    {
        return visitor(e);
    }
    catch (const std::bad_cast& e)
    {
        return visitor(e);
    }
    catch (const std::length_error& e)
    {
        return visitor(e);
    }
    catch (const std::out_of_range& e)
    {
        return visitor(e);
    }
    catch (const std::invalid_argument& e)
    {
        return visitor(e);
    }
    catch (const std::exception& e)
    {
        return visitor(e);
    }
    catch (...)
    {
        return visitor(ep);
    }
}


class exceptionStackIterator
{
public:
    using value_type = std::exception_ptr;

    exceptionStackIterator() noexcept = default;

    exceptionStackIterator(std::exception_ptr exception) noexcept 
        : m_exception(std::move(exception)) 
    {}

    exceptionStackIterator& operator++() noexcept
    {
        try
        {
            std::rethrow_exception(m_exception);
        }
        catch (const std::nested_exception& e)
        {
            m_exception = e.nested_ptr();
        }
        catch (...)
        {
            m_exception = {};
        }
        return *this;
    }

    std::exception_ptr operator*() const noexcept
    {
        return m_exception;
    }

private:
    friend bool operator==(const exceptionStackIterator& a, const exceptionStackIterator& b) noexcept
    {
        return *a == *b;
    }

    friend bool operator!=(const exceptionStackIterator& a, const exceptionStackIterator& b) noexcept
    {
        return !(a == b);
    }


private:
    std::exception_ptr m_exception;
};


class exceptionStackRange
{
public:
    exceptionStackRange() noexcept = default;

    exceptionStackRange(exceptionStackIterator begin, exceptionStackIterator end) noexcept 
        : m_begin(std::move(begin))
        , m_end(std::move(end)) 
    {}

    exceptionStackIterator begin() const noexcept
    {
        return m_begin;
    }

    exceptionStackIterator end() const noexcept
    {
        return m_end;
    }

private:
    exceptionStackIterator m_begin;
    exceptionStackIterator m_end;
};


inline exceptionStackRange makeExceptionStackRange(std::exception_ptr exception) noexcept
{
    return exceptionStackRange(exceptionStackIterator(std::move(exception)), exceptionStackIterator());
}

inline exceptionStackRange currentExceptionStack() noexcept
{
    return makeExceptionStackRange(std::current_exception());
}


} // namespace Er {}
