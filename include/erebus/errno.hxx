#pragma once

#include <erebus/erebus.hxx>

namespace Er
{


class EREBUS_EXPORT ErrnoGuard
    : public Er::NonCopyable
{
public:
    ~ErrnoGuard() noexcept;
    ErrnoGuard() noexcept;
    
private:
    int m_errno;
};


} // namespace Er {}
