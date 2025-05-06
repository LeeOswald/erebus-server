#include <erebus/rtl/error.hxx>
#include <erebus/rtl/property_format.hxx>
#include <erebus/rtl/system/posix_error.hxx>
#if ER_WINDOWS
    #include <erebus/rtl/system/win32_error.hxx>
#endif
#include <erebus/rtl/util/unknown_base.hxx>

#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>


namespace Er
{

namespace
{

struct ErrorCategories
{
    std::shared_mutex mutex;
    std::map<std::string_view, IErrorCategory const*> map;
};

ErrorCategories& registry()
{
    static ErrorCategories reg;
    return reg;
}


} // namespace {}


ER_RTL_EXPORT void registerErrorCategory(std::string_view name, IErrorCategory* cat)
{
    auto& reg = registry();

    std::unique_lock l(reg.mutex);
    [[maybe_unused]] auto res = reg.map.insert({ name, cat });
    ErAssert(res.second);
}

ER_RTL_EXPORT void unregisterErrorCategory(IErrorCategory* cat) noexcept
{
    auto& reg = registry();

    std::unique_lock l(reg.mutex);

    for (auto it = reg.map.begin(); it != reg.map.end(); ++it)
    {
        if (it->second == cat)
        {
            reg.map.erase(it);
            break;
        }
    }
}

ER_RTL_EXPORT IErrorCategory const* lookupErrorCategory(std::string_view name) noexcept
{
    auto& reg = registry();

    std::shared_lock l(reg.mutex);
    auto it = reg.map.find(name);
    if (it == reg.map.end())
        return nullptr;

    return it->second;
}


struct PosixErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static constexpr std::string_view Name = { "POSIX" };

    PosixErrorCategory()
    {
        registerErrorCategory(Name, this);
    }

    std::string_view name() const noexcept
    {
        return Name;
    }

    std::string message(Error::Code code) const
    {
        return System::posixErrorToString(code);
    }
};

static PosixErrorCategory g_PosixError;

ER_RTL_EXPORT IErrorCategory const* const PosixError = &g_PosixError;


#if ER_WINDOWS

struct Win32ErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static constexpr std::string_view Name = { "Win32" };

    Win32ErrorCategory()
    {
        registerErrorCategory(Name, this);
    }

    std::string_view name() const noexcept
    {
        return Name;
    }

    std::string message(Error::Code code) const
    {
        return System::win32ErrorToString(code);
    }
};

static Win32ErrorCategory g_Win32Error;

ER_RTL_EXPORT IErrorCategory const* const Win32Error = &g_Win32Error;

#endif // ER_WINDOWS


bool Error::decode()
{
    if (m_code != Success)
    {
        ErAssert(m_category);
        auto msg = m_category->message(m_code);
        if (!msg.empty())
        {
            m_properties.push_back(ErrorProperties::Message(std::move(msg)));
            return true;
        }
    }

    return false;
}


ER_RTL_EXPORT void logError(Log::ILogger* log, Log::Level level, const Error& e)
{
    if (!e)
        return;

    const std::string* brief = nullptr;
    const std::string* message = nullptr;

    auto extraProps = e.properties().size();

    auto brief_ = findProperty(e.properties(), ErrorProperties::Brief::Name, Property::Type::String);
    if (brief_)
    {
        brief = brief_->getString();
        --extraProps;
    }

    auto message_ = findProperty(e.properties(), ErrorProperties::Message::Name, Property::Type::String);
    if (message_)
    {
        message = message_->getString();
        --extraProps;
    }

    std::ostringstream ss;

    if (brief)
    {
        ss << *brief << ": ";
    }

    ErAssert(e.category());
    ss << "[" << e.category()->name() << " " << e.code() << "]";

    if (message)
    {
        ss << " " << *message;
    }

    if (!extraProps)
    {
        Log::writeln(log, level, ss.str());
        return;
    }

    Log::AtomicBlock a(log);
    Log::writeln(log, level, ss.str());
    Log::IndentScope is(log, level);

    for (auto& prop : e.properties())
    {
        if ((&prop == brief_) || (&prop == message_))
            continue;

        auto fmt = findPropertyFormatter(prop.semantics());
        ErAssert(fmt);

        Log::write(log, level, "{}: {}", prop.nameStr(), fmt(prop));
    }
}

} // namespace Er {}