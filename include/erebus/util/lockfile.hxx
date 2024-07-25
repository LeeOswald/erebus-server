#pragma once

#include <erebus/util/generichandle.hxx>


namespace Er
{

namespace Util
{

class LockFile 
    : public Er::NonCopyable
{
public:
    ~LockFile();
    explicit LockFile(const std::string& path);

    void put(std::string_view data);

private:
    std::string m_path;
#if ER_WINDOWS
    std::wstring m_wpath;
#endif
    FileHandle m_file;
};


} // namespace Util {}

} // namespace Er {}
