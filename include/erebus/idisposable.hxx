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


struct DisposableDisposer final
{
    void operator()(IDisposable* d) noexcept
    {
        if (d) [[likely]]
            d->dispose();
    }
};


template <class T>
using DisposablePtr = std::unique_ptr<T, DisposableDisposer>;


} // namespace Er {}


