#include <erebus/rtl/system/win32_error.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>
#include <erebus/rtl/util/utf16.hxx>


namespace Er::Util
{

ER_RTL_EXPORT ResultCode loadBinaryFile(const std::string& path, Binary& out) noexcept
{
    FileHandle file(::CreateFileW(Er::Util::utf8ToUtf16(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0));
    if (!file.valid())
    {
        auto err = System::resultFromWin32Error(::GetLastError());
        return err ? *err : Result::Failure;
    }

    LARGE_INTEGER li;
    if (!::GetFileSizeEx(file, &li))
    {
        auto err = System::resultFromWin32Error(::GetLastError());
        return err ? *err : Result::Failure;
    }

    if (li.QuadPart > std::numeric_limits<std::uint32_t>::max())
    {
        // file too large
        return Result::InsufficientResources;
    }

    auto size = static_cast<std::uint32_t>(li.QuadPart);
    std::string& bytes = out.bytes();

    try
    {
        bytes.resize(size);
    }
    catch (std::bad_alloc&)
    {
        return Result::OutOfMemory;
    }

    DWORD rd = 0;
    if (!::ReadFile(file, bytes.data(), size, &rd, nullptr))
    {
        auto err = System::resultFromWin32Error(::GetLastError());
        return err ? *err : Result::Failure;
    }

    if (rd < size)
        bytes.resize(rd);

    return Result::Ok;
}

} // namespace Er::Util {}