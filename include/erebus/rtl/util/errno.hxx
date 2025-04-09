#pragma once

#include <erebus/rtl/platform.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Util
{


class ErrnoGuard
    : public boost::noncopyable
{
public:
    ~ErrnoGuard() noexcept
    {
        errno = m_errno;
    }
    
    ErrnoGuard() noexcept
        : m_errno(errno)
    {}
    
private:
    int m_errno;
};


} // namespace Er::Util {}
