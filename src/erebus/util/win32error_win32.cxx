#include <erebus/util/autoptr.hxx>
#include <erebus/util/win32error.hxx>

namespace Er
{

namespace Util
{
    
namespace
{

template <typename T>
struct HeapDeleter
{
    void operator()(T* ptr) noexcept
    {
        ::HeapFree(::GetProcessHeap(), 0, ptr);
    }
};

} // namespace {}


EREBUS_EXPORT std::string win32ErrorToString(DWORD r, HMODULE module)
{
    if (r == 0)
        return std::string();

    AutoPtr<char, HeapDeleter<char>> buffer;
    auto cch = ::FormatMessageA(
        (module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM) | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        module,
        r,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        reinterpret_cast<char*>(buffer.writeable()),
        0,
        nullptr
    );

    std::string s;

    if (cch > 0)
    {
        s = std::string(buffer.get(), cch);

        // Windows appends \r\n to error messages for some reason
        while (s.size() && (s[s.size() - 1] == '\n' || s[s.size() - 1] == '\r'))
        {
            s.erase(s.size() - 1);
        }
    }

    return s;
}

    
    
} // namespace Util {}

} // namespace Er {}
