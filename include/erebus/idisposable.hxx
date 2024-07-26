#pragma once

#include <erebus/erebus.hxx>


namespace Er
{
    
struct IDisposable
{
    virtual void dispose() noexcept = 0; 

protected:
    virtual ~IDisposable() {}
};


namespace __
{

struct Disposer final
{
    void operator()(IDisposable* d) noexcept
    {
        if (d) [[likely]]
            d->dispose();
    }
};

} // namespace __ {}


template <class T>
using DisposablePtr = std::unique_ptr<T, __::Disposer>;


} // namespace Er {}


