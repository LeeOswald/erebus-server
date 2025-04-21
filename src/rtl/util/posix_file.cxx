#include <erebus/rtl/system/posix_error.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>

#include <sys/file.h>
#include <unistd.h>

namespace Er::Util
{

ER_RTL_EXPORT ResultCode loadBinaryFile(const std::string& path, Binary& out) noexcept
{
    FileHandle file(::open(path.c_str(), O_RDONLY));
    if (!file.valid())
    {
        auto err = System::resultFromPosixError(errno);
        return err ? *err : Result::Failure;
    }

    auto size64 = ::lseek64(file, 0, SEEK_END);
    if (size64 == static_cast<decltype(size64)>(-1))
    {
        auto err = System::resultFromPosixError(errno);
        return err ? *err : Result::Failure;
    }

    if (size64 > std::numeric_limits<std::uint32_t>::max())
    {
        // file too large
        return Result::InsufficientResources;
    }

    auto size = static_cast<std::uint32_t>(size64);

    std::string& bytes = out.bytes();

    try
    {
        bytes.resize(size);
    }
    catch (std::bad_alloc&)
    {
        return Result::OutOfMemory;
    }

    ::lseek64(file, 0, SEEK_SET);

    auto rd = ::read(file, bytes.data(), size);
    if (rd < 0)
    {
        auto err = System::resultFromPosixError(errno);
        return err ? *err : Result::Failure;
    }

    if (static_cast<decltype(size)>(rd) < size)
    {
        bytes.resize(rd);
    }

    return Result::Ok;
}

} // namespace Er::Util {}