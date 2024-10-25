#pragma once

#include <erebus/flags.hxx>

#include <iomanip>
#include <sstream>


namespace Er::Log
{

struct EREBUS_EXPORT SimpleFormatter
{
public:
    struct Option
    {
        static constexpr Flag DateTime = 0;
        static constexpr Flag Time = 1;
        static constexpr Flag Level = 2;
        static constexpr Flag Tid = 3;
        static constexpr Flag TzUtc = 4;   // default is local time 
        static constexpr Flag TzCrLf = 5;  // default is LF
    };

    using Options = FlagsPack<32, Option>;

    static IFormatter::Ptr make(Options options = Options{ Option::DateTime, Option::Time, Option::Level, Option::Tid }, unsigned indentSize = 4);
};


} // namespace Er::Log {}