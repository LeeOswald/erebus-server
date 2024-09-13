#pragma once

#include <erebus/log.hxx>
#include <erebus/property.hxx>
#include <erebus/result.hxx>
#include <erebus/util/crc32.hxx>
#include <erebus/util/posixerror.hxx>
#if ER_WINDOWS
    #include <erebus/util/win32error.hxx>
#endif

//
// exception class with (almost) arbitrary properties
// that can be marshaled through RPC
//

namespace Er
{

namespace ExceptionProps
{

namespace Private
{

void registerAll(Er::Log::ILog* log);
void unregisterAll(Er::Log::ILog* log);

} // namespace Private {}

constexpr const std::string_view Domain = "exception";

using ResultCode = PropertyValue<int32_t, ER_PROPID("erebus_result_code"), "Erebus error code", ResultFormatter>;
using DecodedError = PropertyValue<std::string, ER_PROPID("decoded_error"), "Error message">;
using FileName = PropertyValue<std::string, ER_PROPID("file_name"), "File name">;
using DirectoryName = PropertyValue<std::string, ER_PROPID("directory_name"), "Directory name">;
using PosixErrorCode = PropertyValue<int32_t, ER_PROPID("posix_error_code"), "POSIX error code">;
using Win32ErrorCode = PropertyValue<uint32_t, ER_PROPID("win32_error_code"), "WIN32 error code">;


} // ExceptionProps {}


//
// EREBUS_EXPORT is necessary here for typeid() to be able to find 
// exception instance type across modules
//

class EREBUS_EXPORT Exception
    : public std::exception
{
public:
    Exception() = default;

    template <typename MessageT>
    explicit Exception(Location&& location, MessageT&& message) noexcept
    {
        try
        {
            m_context.reset(new Context(std::forward<MessageT>(message)));
            m_context->setLocation(std::move(location));
        }
        catch (...)
        {
            // avoid throwing from the exception constructor
        }
    }

    template <typename MessageT, typename PropT, typename... ExceptionProps>
    explicit Exception(Location&& location, MessageT&& message, PropT&& prop, ExceptionProps&&... props) noexcept
    {
        try
        {
            m_context = std::make_shared<Context>(std::forward<MessageT>(message), std::forward<PropT>(prop), std::forward<ExceptionProps>(props)...);
            m_context->setLocation(std::move(location));
        }
        catch (...)
        {
            // avoid throwing from the exception constructor
        }
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

    template <typename PropT>
        requires std::is_same_v<std::remove_cvref_t<PropT>, Property>
    Exception& add(PropT&& prop)
    {
        if (m_context)
        {
            m_context->addProp(std::forward<PropT>(prop));
        }

        return *this;
    }

    template <SupportedPropertyType ValueT>
    Exception& add(PropId id, ValueT&& value)
    {
        if (m_context)
        {
            m_context->addProp(id, std::forward<ValueT>(value));
        }

        return *this;
    }

    template <IsPropertyValue PropT>
    Exception& add(PropT&& prop)
    {
        if (m_context)
        {
            m_context->addProp(std::forward<PropT>(prop));
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

        template <typename PropT>
            requires std::is_same_v<std::remove_cvref_t<PropT>, Property>
        void addProp(PropT&& prop)
        {
            properties.emplace_back(std::forward<PropT>(prop));
        }

        template <IsPropertyValue PropT>
        void addProp(PropT&& prop)
        {
            properties.emplace_back(std::forward<PropT>(prop));
        }

        template <SupportedPropertyType ValueT>
        void addProp(PropId id, ValueT&& value)
        {
            properties.emplace_back(id, std::forward<ValueT>(value));
        }
    };

    std::shared_ptr<Context> m_context;
};


} // namespace Er {}


#define ErThrowPosixError(msg, err, ...) \
    throw ::Er::Exception(ER_HERE(), msg, ::Er::ExceptionProps::PosixErrorCode(int32_t(err)), ::Er::ExceptionProps::DecodedError(::Er::Util::posixErrorToString(err)), ##__VA_ARGS__)

#if ER_WINDOWS

#define ErThrowWin32Error(msg, err, ...) \
    throw ::Er::Exception(ER_HERE(), msg, ::Er::ExceptionProps::Win32ErrorCode(int32_t(err)), ::Er::ExceptionProps::DecodedError(::Er::Util::win32ErrorToString(err)), ##__VA_ARGS__)

#endif

#define ErThrow(msg, ...) \
    throw ::Er::Exception(ER_HERE(), msg, ##__VA_ARGS__)
