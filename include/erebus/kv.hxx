#pragma once

#include <erebus/rtl/rtl.hxx>

#include <vector>


namespace Er
{

struct KvArray final
{
    struct Entry
    {
        std::string key;
        std::string value;

        constexpr Entry() noexcept = default;

        constexpr Entry(auto&& k, auto&& v)
            : key(std::forward<decltype(k)>(k))
            , value(std::forward<decltype(v)>(v))
        {
        }
    };

    std::vector<Entry> v;
};


} // namespace Er {}