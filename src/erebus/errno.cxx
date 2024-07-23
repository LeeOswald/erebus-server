#include <erebus/errno.hxx>


namespace Er
{

ErrnoGuard::ErrnoGuard() noexcept
    : m_errno(errno)
{}

ErrnoGuard::~ErrnoGuard() noexcept
{
    errno = m_errno;
}


} // namespace Er {}