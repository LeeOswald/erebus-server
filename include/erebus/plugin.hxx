#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/kv.hxx>
#include <erebus/rtl/log.hxx>


namespace Er
{


struct IPlugin
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IPlugin";

    using Ptr = std::unique_ptr<IPlugin>;

    virtual KvArray info() const = 0;

protected:
    friend class Ptr;
    virtual ~IPlugin() = default;
};



// must be exported by any plugin
typedef IPlugin::Ptr (createPlugin)(Log::ILogger* log, IUnknown* host, const KvArray& args);


} // namespace Er {}