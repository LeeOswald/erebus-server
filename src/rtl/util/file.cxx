#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/file.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/system/win32_error.hxx>
    #include <erebus/rtl/system/unwindows.h>
    #include <erebus/rtl/util/utf16.hxx>
#elif ER_POSIX
    #include <erebus/rtl/system/posix_error.hxx>
#endif

#include <cstdio>
#include <filesystem>


namespace Er::Util
{


ER_RTL_EXPORT Binary loadFile(const std::string& path)
{
#if ER_POSIX
    auto result = tryLoadFile(path);
    if (!result.has_value())
        ErThrowPosixError(Er::format("Failed to open {}", path), -result.error());
#elif ER_WINDOWS
    auto result = tryLoadFile(path, out);
    if (!result.has_value())
        ErThrowWin32Error(Er::format("Failed to open {}", path), result.error());
#endif

    return result.value();
}

ER_RTL_EXPORT std::optional<std::string> resolveSymlink(const std::string& path, unsigned maxDepth) noexcept
{
    std::filesystem::path fspath(path);
    std::error_code ec;
    auto link = std::filesystem::is_symlink(fspath, ec);

    if (ec)
        return std::nullopt;

    if (!link)
    {
#if ER_WINDOWS
        auto& native = fspath.native();
        return Er::Util::utf16To8bit(CP_UTF8, native.data(), native.length());
#else
        return fspath.native();
#endif
    }
    
    while (maxDepth)
    {
        fspath = std::filesystem::read_symlink(fspath, ec);
        
        link = std::filesystem::is_symlink(fspath, ec);

        if (ec)
            return std::nullopt;

        if (!link)
        {
#if ER_WINDOWS
            auto& native = fspath.native();
            return Er::Util::utf16To8bit(CP_UTF8, native.data(), native.length());
#else
            return fspath.native();
#endif
        }

        --maxDepth;
    }

    // too many nested links
    return std::nullopt;
}


} // namespace Er::Util {]