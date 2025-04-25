#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Er::Util
{

ER_RTL_EXPORT std::expected<Binary, int> tryLoadFile(const std::string& path) noexcept
{
    FileHandle file(::open(path.c_str(), O_RDONLY));
    if (!file.valid())
    {
        return std::unexpected(errno);
    }

    struct ::stat64 fileStat;
    if (::stat64(path.c_str(), &fileStat) == -1)
    {
        return std::unexpected(errno);
    }

    auto size = static_cast<std::size_t>(fileStat.st_size);

    Binary out;
    std::string& bytes = out.bytes();

    try
    {
        bytes.resize(size);
    }
    catch (std::bad_alloc&)
    {
        return std::unexpected(ENOMEM);
    }

    auto rd = ::read(file, bytes.data(), size);
    if (rd < 0)
    {
        return std::unexpected(errno);
    }

    if (static_cast<decltype(size)>(rd) < size)
    {
        bytes.resize(rd);
    }

    return {std::move(out)};
}

} // namespace Er::Util {}