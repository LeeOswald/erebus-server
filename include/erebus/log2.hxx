#pragma once

#include <erebus/system/time.hxx>

namespace Er::Log2
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Off // should go last
};


struct Record
{
    Level level = Level::Info;
    System::Time time;
    uintptr_t tid = 0;
    std::string message;

    Record() noexcept = default;

    template <typename MessageT>
    explicit Record(Level level, const System::Time& time, uintptr_t tid, MessageT&& message)
        : level(level)
        , time(time)
        , tid(tid)
        , message(std::forward<MessageT>(message))
    {
    }
};

} // namespace Er::Log2 {}