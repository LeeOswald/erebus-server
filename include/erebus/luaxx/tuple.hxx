#pragma once

#include "selector.hxx"

#include <tuple>


namespace Luaxx 
{

template <typename... T>
class Tuple 
{
private:
    std::tuple<T&...> _tuple;

public:
    Tuple(T&... args) 
        : _tuple(args...) 
    {}

    void operator=(const Luaxx::Selector& s) 
    {
        _tuple = s.GetTuple<typename std::remove_reference<T>::type...>();
    }
};

template <typename... T>
Tuple<T&...> tie(T&... args) 
{
    return Tuple<T&...>(args...);
}


} // namespace Luaxx {}
