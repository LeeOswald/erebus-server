#pragma once

#include <erebus/rtl/util/generic_handle.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Util
{

class ER_RTL_EXPORT LockFile 
    : public boost::noncopyable
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


} // namespace Er::Util {}
