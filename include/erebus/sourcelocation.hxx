#pragma once


#if defined(__GNUC__) && (__GNUC__ < 11)
    #include <experimental/source_location>

    namespace Er
    {
        using SourceLocation = std::experimental::source_location;

    } // namespace Er {}

#else
    #include <source_location>

    namespace Er
    {
        using SourceLocation = std::source_location;

    } // namespace Er {}

#endif



