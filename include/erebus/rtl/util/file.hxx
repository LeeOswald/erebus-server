#pragma once

#include <erebus/rtl/binary.hxx>

#include <expected>
#include <filesystem>
#include <vector>
#include <unordered_set>


namespace Er::Util
{

#if ER_POSIX
ER_RTL_EXPORT std::expected<Binary, int> tryLoadFile(const std::string& path) noexcept;
#elif ER_WINDOWS
ER_RTL_EXPORT std::expected<Binary, std::uint32_t> tryLoadFile(const std::string& path) noexcept;
#endif

ER_RTL_EXPORT Binary loadFile(const std::string& path);

ER_RTL_EXPORT std::expected<std::string, int> resolveSymlink(const std::string& path, unsigned maxDepth = 8) noexcept;


} // namespace Er::Util {]