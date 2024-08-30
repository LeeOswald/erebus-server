#pragma once

#include <erebus/erebus.hxx>


namespace Er
{


struct EREBUS_EXPORT Binary final
{
    Binary() noexcept
    {}

    Binary(const Binary& b)
        : m_bytes(b.m_bytes)
    {}

    Binary(Binary&& b) noexcept
        : m_bytes(std::move(b.m_bytes))
    {}

    template <typename StringT>
    explicit Binary(StringT&& s) noexcept
        : m_bytes(std::forward<StringT>(s))
    {}

    Binary& operator=(const Binary& o)
    {
        Binary tmp(o);
        m_bytes.swap(tmp.m_bytes);
        return *this;
    }

    Binary& operator=(Binary&& o) noexcept
    {
        Binary tmp(std::move(o));
        m_bytes.swap(tmp.m_bytes);
        return *this;
    }

    friend auto operator==(const Binary& a, const Binary& b) noexcept
    {
        return a.m_bytes == b.m_bytes;
    }

    friend auto operator<=>(const Binary& a, const Binary& b) noexcept
    {
        return a.m_bytes <=> b.m_bytes;
    }

    const std::string& bytes() const noexcept
    {
        return m_bytes;
    }

    const char* data() const noexcept
    {
        return m_bytes.data();
    }

    std::size_t size() const noexcept
    {
        return m_bytes.size();
    }

    bool empty() const noexcept
    {
        return m_bytes.empty();
    }

    template <class OStreamT>
    friend OStreamT& operator<<(OStreamT& ostream, const Binary& bytes)
    {
        static const char HexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        bool first = true;
        for (auto b : bytes.m_bytes)
        {
            if (first)
                first = false;
            else
                ostream << " ";

            uint8_t hi = uint8_t(b) >> 4;
            uint8_t lo = uint8_t(b) & 0x0f;

            ostream << HexDigits[hi] << HexDigits[lo];
        }

        return ostream;
    }

private:
    std::string m_bytes;
};


} // namespace Er {}