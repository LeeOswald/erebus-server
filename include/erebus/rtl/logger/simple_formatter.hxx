#pragma once

#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/log.hxx>

#include <iomanip>
#include <sstream>


namespace Er::Log
{

struct ER_RTL_EXPORT SimpleFormatter
{
    struct Option
    {
        static constexpr Flag DateTime = 0;   
        static constexpr Flag Time = 1;
        static constexpr Flag Level = 2;
        static constexpr Flag Tid = 3;
        static constexpr Flag TzUtc = 4;
        static constexpr Flag TzLocal = 5;
        static constexpr Flag Lf = 6;
        static constexpr Flag CrLf = 7;
        static constexpr Flag NoLf = 8;       // neiter CR nor LF
        static constexpr Flag Component = 9;
    };

    using Options = FlagsPack<32, Option>;
};


ER_RTL_EXPORT [[nodiscard]] FormatterPtr makeSimpleFormatter(
    SimpleFormatter::Options options = SimpleFormatter::Options
    { 
        SimpleFormatter::Option::Time, 
        SimpleFormatter::Option::Level, 
        SimpleFormatter::Option::Tid, 
        SimpleFormatter::Option::TzLocal, 
        SimpleFormatter::Option::Lf, 
        SimpleFormatter::Option::Component 
    }, 
    unsigned indentSize = 4
);

} // namespace Er::Log {}