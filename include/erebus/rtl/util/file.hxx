#pragma once

#include <erebus/rtl/binary.hxx>

#include <filesystem>
#include <vector>
#include <unordered_set>


namespace Er::Util
{

ER_RTL_EXPORT std::string loadTextFile(const std::string& path);
ER_RTL_EXPORT Binary loadBinaryFile(const std::string& path);

ER_RTL_EXPORT std::optional<std::string> resolveSymlink(const std::string& path, unsigned maxDepth = 8) noexcept;


} // namespace Er::Util {]