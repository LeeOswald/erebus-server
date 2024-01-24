#pragma once

#include <erebus/erebus.hxx>

namespace Er
{

namespace Util
{
    
#if ER_WINDOWS

EREBUS_EXPORT std::string utf16To8bit(int cp, const wchar_t* s, std::optional<size_t> length = std::nullopt);
EREBUS_EXPORT std::wstring utf8ToUtf16(const std::string& s);
EREBUS_EXPORT std::wstring localToUtf16(const std::string& s);

#endif    
    
} // namespace Util {}

} // namespace Er {}