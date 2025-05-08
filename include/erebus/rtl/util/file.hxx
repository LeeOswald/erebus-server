#pragma once

#include <erebus/rtl/binary.hxx>
#include <erebus/rtl/error.hxx>

#include <expected>
#include <filesystem>
#include <vector>
#include <unordered_set>


namespace Er::Util
{

ER_RTL_EXPORT std::expected<Binary, Error> tryLoadFile(const std::string& path) noexcept;

ER_RTL_EXPORT Binary loadFile(const std::string& path);

ER_RTL_EXPORT std::expected<std::string, Error> tryResolveSymlink(const std::string& path, unsigned maxDepth = 8) noexcept;


} // namespace Er::Util {]