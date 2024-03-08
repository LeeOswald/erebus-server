#pragma once

#include <erebus/property.hxx>
#include <erebus/sourcelocation.hxx>
#include <erebus/stacktrace.hxx>
#include <erebus/util/crc32.hxx>

//
// exception class with arbitrary properties
// that can be marshaled through RPC
//

namespace Er
{

namespace ExceptionProps
{

namespace Private
{

void registerAll();

} // namespace Private {}


using DecodedError = PropertyValue<std::string, ER_PROPID("decoded_error"), "Error message", PropertyComparator<std::string>, PropertyFormatter<std::string>>;
using PosixErrorCode = PropertyValue<int32_t, ER_PROPID("posix_error_code"), "POSIX error code", PropertyComparator<int32_t>, PropertyFormatter<int32_t>>;
using Win32ErrorCode = PropertyValue<uint32_t, ER_PROPID("win32_error_code"), "WIN32 error code", PropertyComparator<uint32_t>, PropertyFormatter<uint32_t>>;


} // ExceptionProps {}


//
// EREBUS_EXPORT is necessary here for typeid() to be able to find 
// exception instance type across modules
//

class EREBUS_EXPORT Exception
    : public std::exception
{
public:
    struct Location final
    {
        std::optional<SourceLocation> source;
        std::optional<StackTrace> stack;
        std::optional<DecodedStackTrace> decoded;

        Location() noexcept = default;

        template <typename SourceLocationT>
        Location(SourceLocationT&& source) noexcept
            : source(std::forward<SourceLocationT>(source))
        {}

        Location(SourceLocation&& source, StackTrace&& stack) noexcept
            : source(std::move(source))
            , stack(std::move(stack))
        {
        }

        Location(SourceLocation&& source, DecodedStackTrace&& decoded) noexcept
            : source(std::move(source))
            , decoded(std::move(decoded))
        {
        }
    };

    Exception() = default;

    template <typename MessageT>
    explicit Exception(Location&& location, MessageT&& message)
        : m_context(std::make_shared<Context>(std::forward<MessageT>(message)))
    {
        m_context->setLocation(std::move(location));
    }

    template <typename MessageT, typename PropT, typename... ExceptionProps>
    explicit Exception(Location&& location, MessageT&& message, PropT&& prop, ExceptionProps&&... props)
        : m_context(std::make_shared<Context>(std::forward<MessageT>(message), std::forward<PropT>(prop), std::forward<ExceptionProps>(props)...))
    {
        m_context->setLocation(std::move(location));
    }

    const char* what() const noexcept override
    {
        if (m_context)
            return m_context->message.c_str();
        return "Unknown exception";
    }

    const std::string* message() const noexcept
    {
        if (m_context)
            return &m_context->message;
        return nullptr;
    }

    const Location* location() const noexcept
    {
        if (m_context)
            return &m_context->location;
        return nullptr;
    }

    const std::vector<Property>* properties() const noexcept
    {
        if (m_context)
            return &m_context->properties;
        return nullptr;
    }

    const Property* find(PropId id) const noexcept
    {
        if (!m_context)
            return nullptr;

        for (auto& prop: m_context->properties)
        {
            if (prop.id == id)
                return &prop;
        }

        return nullptr;
    }

    template <typename ValueT>
    Exception& add(PropId id, ValueT&& value)
    {
        if (m_context)
        {
            m_context->addProp(id, std::forward<ValueT&&>(value));
        }

        return *this;
    }

    template <typename PropT>
    Exception& add(PropT&& prop)
    {
        if (m_context)
        {
            m_context->addProp(prop.id(), (std::forward<PropT>(prop)).value());
        }

        return *this;
    }

private:
    struct Context final
    {
        std::string message;
        std::vector<Property> properties;
        Location location;

        template <typename MessageT>
        Context(MessageT&& message)
            : message(std::forward<MessageT>(message))
        {}

        template <typename MessageT, typename PropT, typename... ExceptionProps>
        Context(MessageT&& message, PropT&& prop, ExceptionProps&&... props)
            : Context(std::forward<MessageT>(message), std::forward<ExceptionProps>(props)...)
        {
            addProp(std::forward<PropT>(prop));
        }

        void setLocation(Location&& location)
        {
            this->location = location;
        }

        template <typename ValueT>
        void addProp(PropId id, ValueT&& value)
        {
            properties.emplace_back(id, std::forward<ValueT>(value));
        }

        template <typename PropT>
        void addProp(PropT&& prop)
        {
            properties.emplace_back(prop.id(), (std::forward<PropT>(prop)).value());
        }
    };

    std::shared_ptr<Context> m_context;
};


} // namespace Er {}


#define ER_HERE() ::Er::Exception::Location(::Er::SourceLocationImpl::current(), ::Er::StackTrace())
#define ER_HERE2(skip) ::Er::Exception::Location(::Er::SourceLocationImpl::current(), ::Er::StackTrace(skip, static_cast<std::size_t>(-1)))
#define ER_SOURCE() ::Er::Exception::Location(::Er::SourceLocationImpl::current())



