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
            : k(std::forward_as_tuple<decltype(k)>(k))
            , v(std::forward_as_tuple<decltype(v)>(v))
        {
        }
    };

    std::vector<Entry> v;
};


} // namespace Er {}