#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>
#include <erebus/rtl/util/utf16.hxx>

#include <array>

namespace Er::Util
{

ER_RTL_EXPORT std::expected<Binary, Error> tryLoadFile(const std::string& path) noexcept
{
    FileHandle file(::CreateFileW(Er::Util::utf8ToUtf16(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0));
    if (!file.valid())
    {
        return std::unexpected(Error(::GetLastError(), Win32Error));
    }

    LARGE_INTEGER li;
    if (!::GetFileSizeEx(file, &li))
    {
        return std::unexpected(Error(::GetLastError(), Win32Error));
    }

    auto sizeHint = static_cast<std::size_t>(li.QuadPart);

    const std::size_t blockSize = 8192;
    std::array<char, blockSize> buffer;

    Binary out;
    std::string& bytes = out.bytes();

    try
    {
        if (sizeHint > 0)
        {
            bytes.reserve(sizeHint);
        }

        DWORD rd = 0;
        do
        {
            if (!::ReadFile(file, buffer.data(), buffer.size(), &rd, nullptr))
            {
                return std::unexpected(Error(::GetLastError(), Win32Error));
            }

            if (rd > 0)
            {
                bytes.append(buffer.data(), rd);
            }

        } while (rd > 0);
    }
    catch (std::bad_alloc&)
    {
        return std::unexpected(Error(ERROR_NOT_ENOUGH_MEMORY, Win32Error));
    }

    return {std::move(out)};
}

} // namespace Er::Util {}