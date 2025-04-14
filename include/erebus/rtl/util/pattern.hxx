#pragma once

#include <erebus/rtl/rtl.hxx>

#include <iterator>

namespace Er::Util
{

template <class CharT>
constexpr bool matchString(const CharT* sourceStart, std::size_t sourceLength, const CharT* patternStart, std::size_t patternLength) noexcept
{
    const auto sourceEnd = sourceStart + sourceLength;
    const auto patternEnd = patternStart + patternLength;

    auto current = sourceStart;
    auto patternNext = patternStart;
    auto sourceNext = current;

    while (patternStart < patternEnd || current != sourceEnd) 
    {
        if (patternStart != patternEnd) 
        {
            switch (*patternStart) 
            {
            default:  // Match an ordinary character.
                if (current != sourceEnd && *current == *patternStart) 
                {
                    ++patternStart;
                    ++current;
                    continue;
                }
                break;

            case CharT{ '?' }:  // Match any single character.
                if (current != sourceEnd) 
                {
                    ++patternStart;
                    ++current;
                    continue;
                }
                break;

            case CharT{ '*' }:
                // Match zero or more characters. Start by skipping over the wildcard
                // and matching zero characters from source. If that fails, restart and
                // match one more character than the last attempt.
                patternNext = patternStart;
                sourceNext = current + 1;
                ++patternStart;
                continue;
            }
        }

        // Failed to match a character. Restart if possible.
        if (sourceStart < sourceNext && sourceNext <= sourceEnd) 
        {
            patternStart = patternNext;
            current = sourceNext;
            continue;
        }

        return false;
    }

    return true;
}

template <class StringT>
    requires requires(const StringT s)
    {
        typename StringT::value_type;
        { s.data() } -> std::convertible_to<typename StringT::value_type const*>;
        { s.size() } -> std::convertible_to<std::size_t>;
    }
constexpr bool matchString(const StringT& source, const StringT& pattern) noexcept
{
    return matchString(source.data(), source.size(), pattern.data(), pattern.size());
}


} // namespace Er::Util {}