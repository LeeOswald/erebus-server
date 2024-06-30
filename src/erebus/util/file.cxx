#include <erebus/exception.hxx>
#include <erebus/util/autoptr.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#if ER_WINDOWS
    #include <erebus/util/utf16.hxx>
#endif

#include <cstdio>
#include <filesystem>


namespace Er
{

namespace Util
{

namespace
{

struct FileCloser
{
    void operator()(FILE* f)
    {
        std::fclose(f);
    }
};

using File = AutoPtr<FILE, FileCloser>;

} // namespace {}


EREBUS_EXPORT Bytes loadBinaryFile(const std::string& path)
{
    File f(std::fopen(path.c_str(), "rb"));
    if (!f)
    {
        auto e = errno;
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open %s", path.c_str()), Er::ExceptionProps::PosixErrorCode(e));
    }

    std::setvbuf(f, nullptr, _IONBF, 0);

    std::fseek(f, 0, SEEK_END);
    auto size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size <= 0)
    {
        return Bytes();
    }

    std::string buffer;
    buffer.resize(size);

    auto rd = std::fread(buffer.data(), 1, size, f);
    if (rd < size_t(size))
    {
        buffer.resize(rd);
    }

    return Bytes(std::move(buffer));
}

EREBUS_EXPORT std::string loadTextFile(const std::string& path)
{
    auto bytes = loadBinaryFile(path);
    return std::string(std::move(bytes.bytes()));
}

EREBUS_EXPORT std::optional<std::string> resolveSymlink(const std::string& path, unsigned maxDepth) noexcept
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


} // namespace Util {}

} // namespace Er {]