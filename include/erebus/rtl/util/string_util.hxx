#pragma once

#include <erebus/rtl/rtl.hxx>


namespace Er::Util
{

template <class StringT>
StringT ltrim(const StringT& s)
{
    if constexpr (std::is_same_v<typename StringT::value_type, char>)
    {
        size_t start = s.find_first_not_of(" \n\r\t\f\v");
        return (start == StringT::npos) ? StringT() : s.substr(start);
    }
    else if constexpr (std::is_same_v<typename StringT::value_type, wchar_t>)
    {
        size_t start = s.find_first_not_of(L" \n\r\t\f\v");
        return (start == StringT::npos) ? StringT() : s.substr(start);
    }
    else
    {
        ErAssert(!"Unsupported string type");
    }
}

template <class StringT>
StringT rtrim(const StringT& s)
{
    if constexpr (std::is_same_v<typename StringT::value_type, char>)
    {
        size_t end = s.find_last_not_of(" \n\r\t\f\v");
        return (end == StringT::npos) ? StringT() : s.substr(0, end + 1);
    }
    else if constexpr (std::is_same_v<typename StringT::value_type, wchar_t>)
    {
        size_t end = s.find_last_not_of(L" \n\r\t\f\v");
        return (end == StringT::npos) ? StringT() : s.substr(0, end + 1);
    }
    else
    {
        ErAssert(!"Unsupported string type");
    }
}

template <class StringT>
StringT trim(const StringT& s)
{
    return rtrim(ltrim(s));
}

} // namespace Er::Util {}
