#pragma once

#if !ER_PLATFORM_HXX_INCLUDED
    #include <erebus/rtl/rtl.hxx>
#endif


namespace Er
{

/**
* @brief Fixed-size bool
*
* Sometimes we need a bool that has the same size on all platforms and across the wire.
*/

using Bool = std::uint8_t;

constexpr Bool False = 0;
constexpr Bool True = 1;



} // namespace Er {}