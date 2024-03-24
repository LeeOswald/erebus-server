#pragma once

#include <erebus/erebus.hxx>

#include <cwctype>
#include <string_view>
#include <vector>

namespace Er
{
    
namespace Util
{

struct SplitSkipEmptyPartsT {};
constexpr SplitSkipEmptyPartsT SplitSkipEmptyParts;

struct SplitKeepEmptyPartsT {};
constexpr SplitKeepEmptyPartsT SplitKeepEmptyParts;


template <class StringT, class StringViewT, class ModeT>
std::vector<StringT> split(StringT source, StringViewT delimiters, ModeT mode, std::optional<std::size_t> countHint = std::nullopt)
{
    std::vector<StringT> output;
    if (countHint && *countHint)
        output.reserve(*countHint);
        
    size_t first = 0;

    while (first < source.size())
    {
        const auto second = source.find_first_of(delimiters, first);

        if constexpr (std::is_same_v<ModeT, SplitSkipEmptyPartsT>)
        {
            if (first != second)
                output.emplace_back(source.substr(first, second - first));
        }
        else if constexpr (std::is_same_v<ModeT, SplitKeepEmptyPartsT>)
        {
            output.emplace_back(source.substr(first, second - first));
        }
        else
        {
            assert(!"Unsupported split mode");
        }

        if (second == StringViewT::npos)
            break;

        first = second + 1;
    }

    return output;
}

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
        assert(!"Unsupported string type");
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
        assert(!"Unsupported string type");
    }
}

template <class StringT>
StringT trim(const StringT& s)
{
    return rtrim(ltrim(s));
}


template <typename CharT>
struct CharTraitsIgnoreCase;

template <>
struct CharTraitsIgnoreCase<char>
    : public std::char_traits<char>
{
    static bool eq(char c1, char c2) { return std::toupper(c1) == std::toupper(c2); }
    static bool ne(char c1, char c2) { return std::toupper(c1) != std::toupper(c2); }
    static bool lt(char c1, char c2) { return std::toupper(c1) <  std::toupper(c2); }

    static int compare(const char* s1, const char* s2, size_t n)
    {
        while (n-- != 0)
        {
            if (std::toupper(*s1) < std::toupper(*s2)) return -1;
            if (std::toupper(*s1) > std::toupper(*s2)) return 1;

            ++s1;
            ++s2;
        }

        return 0;
    }

    static const char* find(const char* s, size_t n, char a)
    {
        while (n-- > 0)
        {
            if (std::toupper(*s) == std::toupper(a))
                return s;

            ++s;
        }

        return nullptr;
    }
};

template <>
struct CharTraitsIgnoreCase<wchar_t>
    : public std::char_traits<wchar_t>
{
    static bool eq(wchar_t c1, wchar_t c2) { return std::towupper(c1) == std::towupper(c2); }
    static bool ne(wchar_t c1, wchar_t c2) { return std::towupper(c1) != std::towupper(c2); }
    static bool lt(wchar_t c1, wchar_t c2) { return std::towupper(c1) <  std::towupper(c2); }

    static int compare(const wchar_t* s1, const wchar_t* s2, size_t n)
    {
        while (n-- != 0)
        {
            if (std::towupper(*s1) < std::towupper(*s2)) return -1;
            if (std::towupper(*s1) > std::towupper(*s2)) return 1;

            ++s1;
            ++s2;
        }

        return 0;
    }

    static const wchar_t* find(const wchar_t* s, size_t n, wchar_t a)
    {
        while (n-- > 0)
        {
            if (std::towupper(*s) == std::towupper(a))
                return s;
                
            ++s;
        }

        return nullptr;
    }
};

    
} // namespace Util {}
    
} // namespace Er {}