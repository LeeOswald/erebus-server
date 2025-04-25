#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>

#include <array>

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

    const std::size_t blockSize = 8192;
    std::array<char, blockSize> buffer;

    Binary out;
    std::string& bytes = out.bytes();

    try
    {
        if (fileStat.st_size > 0)
        {
            // st_size is only a hint since the file could have been written to
            // or truncated since we opened it
            bytes.reserve(fileStat.st_size);
        }

        ssize_t rd = 0;
        do
        {
            rd = ::read(file, buffer.data(), buffer.size());
            if (rd < 0)
            {
                return std::unexpected(errno);
            }

            if (rd > 0)
            {
                bytes.append(buffer.data(), rd);
            }

        } while (rd > 0);
    }
    catch (std::bad_alloc&)
    {
        return std::unexpected(ENOMEM);
    }

    return {std::move(out)};
}

} // namespace Er::Util {}