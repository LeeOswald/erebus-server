#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/generic_handle.hxx>
#include <erebus/rtl/util/utf16.hxx>


namespace Er::Util
{

ER_RTL_EXPORT std::expected<Binary, std::uint32_t> tryLoadFile(const std::string& path) noexcept
{
    FileHandle file(::CreateFileW(Er::Util::utf8ToUtf16(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0));
    if (!file.valid())
    {
        return std::unexpected(::GetLastError());
    }

    LARGE_INTEGER li;
    if (!::GetFileSizeEx(file, &li))
    {
        return std::unexpected(::GetLastError());
    }

    if (li.QuadPart > std::numeric_limits<std::uint32_t>::max())
    {
        // file too large
        return std::unexpected(ERROR_NOT_ENOUGH_MEMORY);
    }

    auto size = static_cast<std::uint32_t>(li.QuadPart);

    Binary out;
    std::string& bytes = out.bytes();

    try
    {
        bytes.resize(size);
    }
    catch (std::bad_alloc&)
    {
        return std::unexpected(ERROR_NOT_ENOUGH_MEMORY);
    }

    DWORD rd = 0;
    if (!::ReadFile(file, bytes.data(), size, &rd, nullptr))
    {
        return std::unexpected(::GetLastError());
    }

    if (rd < size)
        bytes.resize(rd);

    return out;
}

} // namespace Er::Util {}