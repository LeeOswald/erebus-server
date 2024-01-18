#include <erebus/util/posixerror.hxx>

namespace Er
{

namespace Util
{
    
EREBUS_EXPORT std::string posixErrorToString(int e)
{
    constexpr size_t required = 256;
    char result[required];
    
    auto s = ::strerror_r(e, result, required); // 's' may be or not be the same as 'result'

    return std::string(s);
}
    
    
} // namespace Util {}

} // namespace Er {}
