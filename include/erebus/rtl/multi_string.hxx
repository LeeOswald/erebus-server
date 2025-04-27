#pragma once

#include <erebus/rtl/rtl.hxx>

#include <boost/functional/hash.hpp>


namespace Er
{


/**
 * string that contains one or more parts separated by '\0' or some other delimiter
 */

template <typename _Char, _Char _Delimiter, class _Traits = std::char_traits<_Char>, class _Allocator = std::allocator<_Char>>
struct BasicMultiString final
{
    using Char = _Char;
    using String = std::basic_string<_Char, _Traits, _Allocator>;
    static constexpr Char Delimiter = _Delimiter;

    BasicMultiString() noexcept = default;

    BasicMultiString(auto&& source)
        : raw(std::forward<decltype(source)>(source))
    {}

    constexpr bool operator==(const BasicMultiString& o) const noexcept
    {
        return raw == o.raw;
    }

    constexpr auto operator<=>(const BasicMultiString& o) const noexcept
    {
        return raw <=> o.raw;
    }

    auto hash() const noexcept
    {
        boost::hash<decltype(raw)> h;
        return h(raw);
    }

    String coalesce(Char newDelimiter) const
    {
        if (raw.empty())
            return {};

        auto separatorPos = raw.find(Delimiter);

        String out;
        out.reserve(raw.size());
        decltype(separatorPos) partBegin = 0;
        for (;;)
        {
            if (separatorPos == raw.npos)
            {
                // append the remainder
                auto partEnd = raw.length();
                ErAssert(partBegin < partEnd);
            
                out.append(raw.substr(partBegin, partEnd - partBegin));
 
                break;
            }

            // append part if not empty
            if (separatorPos > partBegin)
            {
                out.append(raw.substr(partBegin, separatorPos - partBegin));
            }

            // append the substituted separator
            out.append(1, newDelimiter);

            partBegin = separatorPos + 1;
            if (partBegin >= raw.length())
                break;

            separatorPos = raw.find(Delimiter, partBegin);
        }

        return out;
    }

    template <typename Callback>
        requires std::is_invocable_v<Callback, Char const*, std::size_t>
    void split(Callback&& callback) const 
    {
        if (raw.empty())
            return;

        auto begin = raw.data();
        auto separatorPos = raw.find(Delimiter);

        decltype(separatorPos) partBegin = 0;
        for (;;)
        {
            if (separatorPos == raw.npos)
            {
                // the remainder
                auto partEnd = raw.length();
                ErAssert(partBegin <= partEnd);
            
                if constexpr (std::is_invocable_r_v<CallbackResult, Callback, Char const*, std::size_t>)
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
            if constexpr (std::is_invocable_r_v<CallbackResult, Callback, Char const*, std::size_t>)
            {
                if (callback(begin + partBegin, separatorPos - partBegin) != CallbackResult::Continue)
                    break;
            }
            else
            {
                callback(begin + partBegin, separatorPos - partBegin);
            }

            partBegin = separatorPos + 1;
            if (partBegin > raw.length())
                break;

            separatorPos = raw.find(Delimiter, partBegin);
        }
    }

    String raw;
};


template <char _Delimiter>
using MultiString = BasicMultiString<char, _Delimiter>;

using MultiStringZ = BasicMultiString<char, '\0'>;


template <typename _Char, _Char _Delimiter, class _Traits = std::char_traits<_Char>, class _Allocator = std::allocator<_Char>>
auto hash_value(const BasicMultiString<_Char, _Delimiter, _Traits, _Allocator>& s) noexcept
{
    return s.hash();
}


} // namespace Er {}
