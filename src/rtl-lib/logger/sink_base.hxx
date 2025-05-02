#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log::Private
{

struct SinkBase
{
    SinkBase(FormatterPtr&& formatter, FilterPtr&& filter) noexcept
        : m_formatter(std::move(formatter))
        , m_filter(std::move(filter))
    {}

    bool filter(const IRecord* r) const 
    {
        if (m_filter && !m_filter->filter(r))
            return false;

        return true;
    }

    std::string format(const IRecord* r) const
    {
        return m_formatter->format(r);
    }

private:
    FormatterPtr m_formatter;
    FilterPtr m_filter;
};

} // namespace Er::Log::Private {}