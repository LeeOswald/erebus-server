#pragma once

#include <erebus/property.hxx>
#include <erebus/sourcelocation.hxx>
#include <erebus/util/crc32.hxx>

#include <boost/stacktrace.hpp>

namespace Er
{

//
// EREBUS_EXPORT is nexessary here for typeid() to be able to find 
// exception instance type across modules
//

class EREBUS_EXPORT Exception
    : public std::exception
{
public:
    Exception() = default;

    template <typename MessageT>
    explicit Exception(SourceLocation source, MessageT&& message)
        : m_source(source)
        , m_context(std::make_shared<Context>(std::forward<MessageT>(message)))
    {}

    template <typename MessageT, typename PropT, typename... ExceptionProps>
    explicit Exception(SourceLocation source, MessageT&& message, PropT&& prop, ExceptionProps&&... props)
        : m_source(source)
        , m_context(std::make_shared<Context>(std::forward<MessageT>(message), std::forward<PropT>(prop), std::forward<ExceptionProps>(props)...))
    {}

    const char* what() const noexcept override
    {
        if (m_context)
            return m_context->message.c_str();
        return "Unknown exception";
    }

    const SourceLocation& source() const noexcept
    {
        return m_source;
    }

    const std::string* message() const noexcept
    {
        if (m_context)
            return &m_context->message;
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
    struct Context
    {
        std::string message;
        std::vector<Property> properties;

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

    SourceLocation m_source;
    std::shared_ptr<Context> m_context;
};


namespace ExceptionProps
{

namespace Private
{

void registerAll();

} // namespace Private {}


using DecodedError = PropertyInfo<std::string, ER_PROPID("decoded_error"), "Error message", PropertyFormatter<std::string>>;
using PosixErrorCode = PropertyInfo<int, ER_PROPID("posix_error_code"), "POSIX error code", PropertyFormatter<int>>;


} // ExceptionProps {}


} // namespace Er {}




