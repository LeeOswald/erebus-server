#pragma once

#include <erebus/rtl/rtl.hxx>


namespace Er::Util
{

template <class _String>
concept StringType = requires(const _String s, const _String sub)
{
    typename _String::value_type;
    { s.data() } -> std::convertible_to<typename _String::value_type const*>;
    { s.length() } -> std::convertible_to<std::size_t>;
    { s.empty() } -> std::convertible_to<bool>;
    { s.find(sub) } -> std::convertible_to<std::size_t>;
    { s.npos } -> std::convertible_to<std::size_t>;
    { s.find_first_not_of(sub) } -> std::convertible_to<std::size_t>;
    { s.find_last_not_of(sub) } -> std::convertible_to<std::size_t>;
};

template <StringType _String>
_String ltrim(const _String& s)
{
    if constexpr (std::is_same_v<typename _String::value_type, char>)
    {
        size_t start = s.find_first_not_of(" \n\r\t\f\v");
        return (start == _String::npos) ? _String() : s.substr(start);
    }
    else if constexpr (std::is_same_v<typename _String::value_type, wchar_t>)
    {
        size_t start = s.find_first_not_of(L" \n\r\t\f\v");
        return (start == _String::npos) ? _String() : s.substr(start);
    }
    else
    {
        ErAssert(!"Unsupported string type");
    }
}

template <StringType _String>
_String rtrim(const _String& s)
{
    if constexpr (std::is_same_v<typename _String::value_type, char>)
    {
        size_t end = s.find_last_not_of(" \n\r\t\f\v");
        return (end == _String::npos) ? _String() : s.substr(0, end + 1);
    }
    else if constexpr (std::is_same_v<typename _String::value_type, wchar_t>)
    {
        size_t end = s.find_last_not_of(L" \n\r\t\f\v");
        return (end == _String::npos) ? _String() : s.substr(0, end + 1);
    }
    else
    {
        ErAssert(!"Unsupported string type");
    }
}

template <StringType _String>
_String trim(const _String& s)
{
    return rtrim(ltrim(s));
}


template <StringType _String, typename _Callback>
    requires std::is_invocable_v<_Callback, typename _String::value_type const*, std::size_t>
void split(_String source, typename _String::value_type delimiter, _Callback&& callback)
{
    if (source.empty())
        return;

    auto begin = source.data();
    auto separatorPos = source.find(delimiter);

    decltype(separatorPos) partBegin = 0;
    for (;;)
    {
        if (separatorPos == source.npos)
        {
            // the remainder
            auto partEnd = source.length();
            ErAssert(partBegin <= partEnd);

            if constexpr (std::is_invocable_r_v<CallbackResult, _Callback, typename _String::value_type const*, std::size_t>)
            {
                if (callback(begin + partBegin, partEnd - partBegin) != CallbackResult::Continue)
                    break;
            }
            else
            {
                callback(begin + partBegin, partEnd - partBegin);
            }

            break;
        }

        // next part
        if constexpr (std::is_invocable_r_v<CallbackResult, _Callback, typename _String::value_type const*, std::size_t>)
        {
            if (callback(begin + partBegin, separatorPos - partBegin) != CallbackResult::Continue)
                break;
        }
        else
        {
            callback(begin + partBegin, separatorPos - partBegin);
        }

        partBegin = separatorPos + 1;
        if (partBegin > source.length())
            break;

        separatorPos = source.find(delimiter, partBegin);
    }
}

} // namespace Er::Util {}
