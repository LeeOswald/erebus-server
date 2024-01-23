#pragma once

#include <erebus/log.hxx>

namespace ErCtl
{

class Log final
    : public Er::Log::LogBase
{
public:
    ~Log();
    explicit Log(Er::Log::Level level);

private:
    void delegate(std::shared_ptr<Er::Log::Record> r);
};


} // namespace ErCtl {}