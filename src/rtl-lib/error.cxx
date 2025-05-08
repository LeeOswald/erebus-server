#include <erebus/rtl/error.hxx>
#include <erebus/rtl/format.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/util/auto_ptr.hxx>
    #include <erebus/rtl/util/utf16.hxx>
#endif

#include <erebus/rtl/util/unknown_base.hxx>

#include <cstring>
#include <future>
#include <map>
#include <mutex>
#include <shared_mutex>



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


struct GenericErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static constexpr std::string_view Name = { "Erebus" };

    GenericErrorCategory()
    {
        registerErrorCategory(Name, this);
    }

    bool local() const noexcept override
    {
        return false; // can be marshaled
    }

    std::string_view name() const noexcept override
    {
        return Name;
    }

    std::string message(Error::Code code) const override
    {
        struct ResultMapping
        {
            ResultCode code;
            std::string message;
        };

        static const ResultMapping map[] =
        {
            { Result::Ok, "Success" },
            { Result::OutOfMemory, "Out of memory" },
            { Result::Internal, "Internal error" },
            { Result::ScriptError, "Script error" },
            { Result::InvalidInput, "Invalid input data" },
            { Result::BadSymlink, "Bad symbolic link" },
            { Result::BadPlugin, "Invalid plugin" },
            { Result::BadConfiguration, "Invalid configuration" },
            { Result::FailedPrecondition, "Precondition not fulfilled" }
        };

        for (auto& m : map)
        {
            if (m.code == code)
                return m.message;
        }

        static std::string empty;
        return empty;
    }
};

static GenericErrorCategory g_GenericError;

IErrorCategory const* const GenericError = &g_GenericError;


struct PosixErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static constexpr std::string_view Name = { "POSIX" };

    PosixErrorCategory()
    {
        registerErrorCategory(Name, this);
    }

    bool local() const noexcept override
    {
        return false; // can be marshaled
    }

    std::string_view name() const noexcept override
    {
        return Name;
    }

    std::string message(Error::Code code) const override
    {
        constexpr size_t required = 256;
        char result[required];
        result[0] = 0;

#if ER_LINUX
        auto s = ::strerror_r(code, result, required); // 's' may be or not be the same as 'result'
        return std::string(s);
#elif ER_WINDOWS
        if (::strerror_s(result, code) == 0)
            return std::string(result);
#endif

        return std::string();
    }
};

static PosixErrorCategory g_PosixError;

IErrorCategory const* const PosixError = &g_PosixError;


#if ER_WINDOWS

struct Win32ErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static constexpr std::string_view Name = { "Win32" };

    Win32ErrorCategory()
    {
        registerErrorCategory(Name, this);
    }

    bool local() const noexcept override
    {
        return false; // can be marshaled
    }

    std::string_view name() const noexcept override
    {
        return Name;
    }

    std::string message(Error::Code code) const override
    {
        return win32ErrorToString(code);
    }

private:
    static std::string win32ErrorToString(DWORD r, HMODULE module = 0)
    {
        if (r == 0)
            return std::string();

        Util::AutoPtr<wchar_t, decltype([](void* ptr) { ::HeapFree(::GetProcessHeap(), 0, ptr); }) > buffer;
        auto cch = ::FormatMessageW(
            (module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM) | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            module,
            r,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            reinterpret_cast<wchar_t*>(buffer.writeable()),
            0,
            nullptr
        );

        std::wstring s;

        if (cch > 0)
        {
            s = std::wstring(buffer.get(), cch);

            // Windows appends ".\r\n" to error messages for some reason
            while (s.size() && (s[s.size() - 1] == L'\n' || s[s.size() - 1] == L'\r'))
            {
                s.resize(s.size() - 1);
            }

            if (s.size() && (s[s.size() - 1] == L'.'))
            {
                s.resize(s.size() - 1);
            }
        }

        return Util::utf16To8bit(CP_UTF8, s.data(), s.length());
    }
};

static Win32ErrorCategory g_Win32Error;

IErrorCategory const* const Win32Error = &g_Win32Error;

#endif // ER_WINDOWS


struct CxxErrorCategory
    : public Util::ObjectBase<IErrorCategory>
{
    static std::string makeName(const std::error_category& cat)
    {
        return std::string("Cxx:") + cat.name();
    }

    CxxErrorCategory(const std::error_category& cat, const std::string& name)
        : m_cat(cat)
        , m_name(name)
    {
    }

    bool local() const noexcept override
    {
        return true; // cannot be marshaled
    }

    std::string_view name() const noexcept
    {
        return m_name;
    }

    std::string message(Error::Code code) const
    {
        return m_cat.message(code);
    }

private:
    const std::error_category& m_cat;
    std::string m_name;
};


ER_RTL_EXPORT IErrorCategory const* registerCxxErrorCategory(std::error_category const& cat)
{
    auto name = CxxErrorCategory::makeName(cat);

    auto& reg = registry();

    std::unique_lock l(reg.mutex);
    auto it = reg.map.find(name);
    if (it != reg.map.end())
        return it->second;

    [[maybe_unused]] auto res = reg.map.insert({ name, new CxxErrorCategory(cat, name) });
    ErAssert(res.second);

    return res.first->second;
}

Error::Error(std::error_code ec)
{
#if ER_WINDOWS
    if (!std::strcmp(std::system_category().name(), ec.category().name()))
    {
        m_code = ec.value();
        m_category = Win32Error;
        return;
    }
#elif ER_POSIX
    if (!std::strcmp(std::system_category().name(), ec.category().name()))
    {
        m_code = ec.value();
        m_category = PosixError;
        return;
    }
#endif

    auto cat = registerCxxErrorCategory(ec.category());

    m_code = ec.value();
    m_category = cat;
}

std::string Error::message() const
{
    if (m_code == Success)
        return { "Success" };

    return Er::format("[{} {}] {}", m_category->name(), m_code, m_category->message(m_code));
}

} // namespace Er {}