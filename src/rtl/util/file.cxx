#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/posix_error.hxx>
#include <erebus/rtl/util/auto_ptr.hxx>
#include <erebus/rtl/util/file.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/system/unwindows.h>
    #include <erebus/rtl/util/utf16.hxx>
#endif

#include <cstdio>
#include <filesystem>


namespace Er::Util
{

namespace
{

using File = AutoPtr<FILE, decltype([](FILE* f) { std::fclose(f); })>;

} // namespace {}


ER_RTL_EXPORT Binary loadBinaryFile(const std::string& path)
{
    File f(std::fopen(path.c_str(), "rb"));
    if (!f)
        ErThrowPosixError(Er::format("Failed top open file {}", path), errno);

    std::setvbuf(f, nullptr, _IONBF, 0);

    std::fseek(f, 0, SEEK_END);
    auto size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size <= 0)
    {
        return Binary();
    }

    std::string buffer;
    buffer.resize(size);

    auto rd = std::fread(buffer.data(), 1, size, f);
    if (rd < size_t(size))
    {
        buffer.resize(rd);
    }

    return Binary(std::move(buffer));
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