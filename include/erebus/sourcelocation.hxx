#pragma once

#include <erebus/erebus.hxx>

#include <source_location>

namespace Er
{

//
// unlike std::source_location, Er::SourceLocation can be marshaled
//

struct SourceLocation
{
    SourceLocation() noexcept = default;
    
    SourceLocation(const std::source_location& source)
        : m_file(source.file_name())
        , m_line(source.line())
    {
    }

    SourceLocation(const SourceLocation& source)
        : m_file(source.m_file)
        , m_line(source.m_line)
    {
    }

    template <typename FileT>
    SourceLocation(FileT&& file, uint32_t line)
        : m_file(std::forward<FileT>(file))
        , m_line(line)
    {
    }

    SourceLocation(SourceLocation&& source) noexcept(noexcept(std::is_nothrow_move_constructible_v<std::string>))
        : m_file(std::move(source.m_file))
        , m_line(source.m_line)
    {
    }

    friend void swap(SourceLocation& a, SourceLocation& b) noexcept(noexcept(std::is_nothrow_swappable_v<std::string>))
    {
        using std::swap;
        swap(a.m_file, b.m_file);
        swap(a.m_line, b.m_line);
    }

    SourceLocation& operator=(const SourceLocation& o)
    {
        SourceLocation tmp(o);
        swap(*this, tmp);
        return *this;
    }

    SourceLocation& operator=(SourceLocation&& o) noexcept(noexcept(std::is_nothrow_move_constructible_v<std::string> && std::is_nothrow_swappable_v<std::string>))
    {
        SourceLocation tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

    const char* file() const noexcept
    {
        return m_file.c_str();
    }

    constexpr uint32_t line() const noexcept
    {
        return m_line;
    }

private:
    std::string m_file;
    uint32_t m_line = 0;
};

} // namespace Er {}

