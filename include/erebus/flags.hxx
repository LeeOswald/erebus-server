#pragma once

#include <erebus/erebus.hxx>

#include <bitset>


namespace Er
{

using Flag = unsigned;


template <std::size_t N>
class FlagsBase
{
public:
    struct FromBitsT {};
    static constexpr FromBitsT FromBits = FromBitsT();
    
    static constexpr std::size_t Size = N;

    constexpr FlagsBase() noexcept = default;

    explicit FlagsBase(std::uint32_t representation, FromBitsT) noexcept
        : m_bits(representation)
    {
        static_assert(Size >= 32);
    }

    explicit FlagsBase(std::uint64_t representation, FromBitsT) noexcept
        : m_bits(representation)
    {
        static_assert(Size >= 64);
    }

    FlagsBase(std::initializer_list<Flag> initializers) noexcept
        : m_bits()
    {
        for (auto& f : initializers)
            set(f);
    }

    template <typename CharT, typename TraitsT>
    explicit FlagsBase(std::basic_string_view<CharT, TraitsT> representation) noexcept
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
            if (representation[SrcSize - index - 1] == static_cast<CharT>('1'))
            {
                set(index);
            }

        } while (index != 0);
    }

    FlagsBase& set(Flag f, bool value = true) noexcept
    {
        assert(f < Size);
        m_bits.set(f, value);
        return *this;
    }

    FlagsBase& reset(Flag f) noexcept
    {
        assert(f < Size);
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
        assert(f < Size);
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
        std::basic_string<CharT, TraitsT, AllocatorT> out;
        out.reserve(N);
        
        for (auto i = N; i > 0; --i)
        {
            auto bit = m_bits[i - 1];
            out.push_back(static_cast<CharT>(bit ? '1' : '0'));
        }

        return out;
    }

    template <typename T>
    std::enable_if_t<(std::is_same_v<T, std::uint32_t> && (N <= 32)), std::uint32_t> pack() const noexcept
    {
        return static_cast<std::uint32_t>(m_bits.to_ulong());
    }

    template <typename T>
    std::enable_if_t<(std::is_same_v<T, std::uint64_t> && (N <= 64)), std::uint64_t> pack() const noexcept
    {
        return static_cast<std::uint64_t>(m_bits.to_ullong());
    }

private:
    std::bitset<N> m_bits;
};


namespace __
{

template <class UserFlagsT>
concept UserFlags =
    requires(UserFlagsT f)
    {
        { UserFlagsT::FlagsCount } -> std::convertible_to<std::size_t>;
    };

} // namespace __ {}


template <__::UserFlags UserFlagsT>
class Flags final
    : public FlagsBase<UserFlagsT::FlagsCount>
    , public UserFlagsT
{
public:
    using Base = FlagsBase<UserFlagsT::FlagsCount>;
    
    static constexpr std::size_t Size = UserFlagsT::FlagsCount;
    static_assert(Size > 0);

    constexpr Flags() noexcept = default;

    Flags(std::initializer_list<Flag> initializers) noexcept
        : Base(initializers)
    {
    }

    template <typename CharT, typename TraitsT>
    explicit Flags(std::basic_string_view<CharT, TraitsT> representation) noexcept
        : Base(representation)
    {
    }

    explicit Flags(std::uint32_t representation, typename Base::FromBitsT f) noexcept
        : Base(representation, f)
    {
        static_assert(Size >= 32);
    }

    explicit Flags(std::uint64_t representation, typename Base::FromBitsT f) noexcept
        : Base(representation, f)
    {
        static_assert(Size >= 64);
    }
};

} // namespace Er{}