#pragma once

#include <erebus/erebus.hxx>

#include <bitset>


namespace Er
{

template <std::size_t N>
class FlagsBase
{
public:
    using Flag = std::size_t;
    static constexpr std::size_t Size = N;

    constexpr FlagsBase() noexcept = default;

    FlagsBase(std::initializer_list<Flag> initializers) noexcept
        : m_bits()
    {
        for (auto& f : initializers)
            set(f);
    }

    template <typename StringT>
    FlagsBase(const StringT& representation) noexcept
        : m_bits()
    {
        if (representation.empty())
            return;

        const auto SrcSize = representation.size();
        assert(SrcSize <= Size);

        auto index = std::min(SrcSize, Size);
        do
        {
            --index;
            if (representation[SrcSize - index - 1] == static_cast<typename StringT::value_type>('1'))
            {
                set(index);
            }

        } while (index != 0);
    }

    FlagsBase& set(Flag f, bool value = true) noexcept
    {
        m_bits.set(f, value);
        return *this;
    }

    FlagsBase& reset(Flag f) noexcept
    {
        set(f, false);
        return *this;
    }

    FlagsBase& reset() noexcept
    {
        m_bits.reset();
        return *this;
    }

    bool operator[](Flag f) const noexcept
    {
        return m_bits[f];
    }

    bool operator==(const FlagsBase& o) const noexcept
    {
        return m_bits == o.m_bits;
    }

    bool operator!=(const FlagsBase& o) const noexcept
    {
        return m_bits != o.m_bits;
    }

    constexpr std::size_t size() const noexcept
    {
        return Size;
    }

    std::size_t count() const noexcept
    {
        return m_bits.count();
    }

    bool any() const noexcept
    {
        return m_bits.any();
    }

    bool none() const noexcept
    {
        return m_bits.none();
    }

    template <class CharT = char, class TraitsT = std::char_traits<CharT>, class AllocatorT = std::allocator<CharT>>
    std::basic_string<CharT, TraitsT, AllocatorT> to_string() const
    {
        return m_bits.to_string<CharT, TraitsT, AllocatorT>();
    }

private:
    std::bitset<N> m_bits;
};


template <class UserFlagsT>
class Flags final
    : public UserFlagsT
{
public:
    using Flag = typename UserFlagsT::Flag;
    static constexpr std::size_t Size = UserFlagsT::Size;

    static_assert(std::is_base_of_v<FlagsBase<Size>, UserFlagsT>);

    constexpr Flags() noexcept = default;

    Flags(std::initializer_list<Flag> initializers) noexcept
        : UserFlagsT(initializers)
    {
    }

    template <typename StringT>
    Flags(const StringT& representation) noexcept
        : UserFlagsT(representation)
    {
    }
};

} // namespace Er{}