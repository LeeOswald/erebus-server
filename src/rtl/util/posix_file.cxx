#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>

#include <sys/file.h>
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

    auto size64 = ::lseek64(file, 0, SEEK_END);
    if (size64 == static_cast<decltype(size64)>(-1))
    {
        return std::unexpected(errno);
    }

    auto size = static_cast<std::size_t>(size64);

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

    ::lseek64(file, 0, SEEK_SET);

    auto rd = ::read(file, bytes.data(), size);
    if (rd < 0)
    {
        return std::unexpected(errno);
    }

    if (static_cast<decltype(size)>(rd) < size)
    {
        bytes.resize(rd);
    }

    return out;
}

} // namespace Er::Util {}