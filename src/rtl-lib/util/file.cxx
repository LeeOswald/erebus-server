#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/file.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/util/utf16.hxx>
#endif

#include <cstdio>
#include <filesystem>


namespace Er::Util
{


ER_RTL_EXPORT Binary loadFile(const std::string& path)
{
    auto result = tryLoadFile(path);
    if (!result.has_value())
    {
        throw Exception(std::source_location::current(), result.error(), ExceptionProperties::ObjectName(path));
    }

    return result.value();
}

ER_RTL_EXPORT std::expected<std::string, Error> tryResolveSymlink(const std::string& path, unsigned maxDepth) noexcept
{
    std::filesystem::path fspath(path);
    for (;;)
    {
        std::error_code ec;
        auto link = std::filesystem::is_symlink(fspath, ec);

        if (ec)
            return std::unexpected(Error(ec));

        if (!link)
        {
#if ER_WINDOWS
            auto& native = fspath.native();
            return Er::Util::utf16To8bit(CP_UTF8, native.data(), native.length());
#else
            return fspath.native();
#endif
        }
    
        if (!maxDepth)
            break;

        fspath = std::filesystem::read_symlink(fspath, ec);
        
        if (ec)
            return std::unexpected(Error(ec));

        --maxDepth;
    }

    // too many nested links
    return std::unexpected(Error(Result::BadSymlink, GenericError));
}


} // namespace Er::Util {]