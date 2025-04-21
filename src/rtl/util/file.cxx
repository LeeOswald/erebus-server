#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/file.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/system/unwindows.h>
    #include <erebus/rtl/util/utf16.hxx>
#endif

#include <cstdio>
#include <filesystem>


namespace Er::Util
{


ER_RTL_EXPORT Binary loadBinaryFile(const std::string& path)
{
    Binary out;
    auto result = loadBinaryFile(path, out);
    if (result != Result::Ok)
        ErThrowResult(Er::format("Failed to open file {}", path), result);

    return out;
}

ER_RTL_EXPORT std::string loadTextFile(const std::string& path)
{
    auto bytes = loadBinaryFile(path);
    return bytes.release();
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