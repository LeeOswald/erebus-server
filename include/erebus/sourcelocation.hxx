#pragma once

#include <erebus/erebus.hxx>

#if defined(__GNUC__) && (__GNUC__ < 11)
    #include <experimental/source_location>

    namespace Er
    {
        using SourceLocationImpl = std::experimental::source_location;

    } // namespace Er {}

#else
    #include <source_location>

    namespace Er
    {
        using SourceLocationImpl = std::source_location;

    } // namespace Er {}

#endif

namespace Er
{

//
// unlike std::source_location, Er::SourceLocation can be marshaled
//

struct SourceLocation
{
    SourceLocation() noexcept = default;
    
    SourceLocation(const SourceLocationImpl& source) noexcept
        : m_file(source.file_name())
        , m_line(source.line())
    {
    }

    template <typename FileT>
    SourceLocation(FileT&& file, uint32_t line)
        : m_file(std::forward<FileT>(file))
        , m_line(line)
    {
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

