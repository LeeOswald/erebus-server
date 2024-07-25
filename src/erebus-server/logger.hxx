#pragma once


#include <erebus/log.hxx>
#include <erebus/util/generichandle.hxx>


namespace Er
{

namespace Private
{

class LogRotator
    : public Er::Log::LogBase
{
public:
    ~LogRotator() {}
    explicit LogRotator(Er::Log::Level level, const char* fileName, int keep);
};


class Logger final
    : public LogRotator
{
public:
    ~Logger();
    explicit Logger(Er::Log::Level level, const char* fileName, int keep);

private:
    void delegate(std::shared_ptr<Er::Log::Record> r);

    Er::Util::FileHandle m_file;
};


} // namespace Private {}

} // namespace Er {}
