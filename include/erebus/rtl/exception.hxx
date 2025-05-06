#pragma once

#include <erebus/rtl/format.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/result.hxx>

#include <stdexcept>
#include <vector>


//
// exception class with (almost) arbitrary properties
// that can be marshaled through RPC
//

namespace Er
{


class ER_RTL_EXPORT Exception
    : public std::exception
{
public:
    template <typename _Message>
        requires std::is_constructible_v<std::string, _Message>
    explicit Exception(std::source_location location, _Message&& message) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<_Message>(message)))
    {
    }

    template <typename _Message, typename _Property, typename... _Properties>
        requires std::is_constructible_v<std::string, _Message> &&
            std::is_constructible_v<Property, _Property> && (std::is_constructible_v<Property, _Properties> && ...)
    explicit Exception(std::source_location location, _Message&& message, _Property&& prop, _Properties&&... props) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<_Message>(message), std::forward<_Property>(prop), std::forward<_Properties>(props)...))
    {
    }

    const char* what() const noexcept override
    {
        return m_context->message.c_str();
    }

    const auto& message() const
    {
        return m_context->message;
    }

    auto location() const noexcept
    {
        return m_context->location;
    }

    const auto& properties() const noexcept
    {
        return m_context->properties;
    }

    Exception& add(auto&& prop)
    {
        m_context->addProp(std::forward<decltype(prop)>(prop), true);
        return *this;
    }

protected:
    struct Context final
    {
        std::source_location location;
        std::string message;
        PropertyBag properties;
       
        Context(std::source_location location, auto&& message)
            : location(location)
            , message(std::forward<decltype(message)>(message))
        {
        }

        Context(std::source_location location, auto&& message, auto&& prop, auto&&... props)
            : Context(location, std::forward<decltype(message)>(message), std::forward<decltype(props)>(props)...)
        {
            addProp(std::forward<decltype(prop)>(prop), false);
        }

        void addProp(auto&& prop, bool back)
        {
            if (back)
                properties.push_back(std::forward<decltype(prop)>(prop));
            else
                properties.insert(properties.begin(), std::forward<decltype(prop)>(prop));
        }
    };

    std::shared_ptr<Context> m_context;
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


namespace ExceptionProps
{

constexpr std::string_view ResultCode{ "result_code" };
constexpr std::string_view DecodedError{ "decoded_error" };

} // namespace ExceptionProps {}


ER_RTL_EXPORT [[nodiscard]] Exception makeExceptionFromResult(std::source_location location, std::string&& message, ResultCode code);

} // namespace Er {}


#define ErThrow(message, ...) \
    throw ::Er::Exception(std::source_location::current(), message, ##__VA_ARGS__)

#define ErThrowResult(message, code) \
    throw ::Er::makeExceptionFromResult(std::source_location::current(), std::move(message), code)

