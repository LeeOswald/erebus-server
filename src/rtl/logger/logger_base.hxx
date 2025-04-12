#pragma once

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/system/thread.hxx>
#include <erebus/rtl/util/thread_data.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Log::Private
{

class LoggerBase
    : public ILogger
    , public boost::noncopyable
{
public:
    ~LoggerBase()
    {
    }

    LoggerBase(std::string_view component, ITee::Ptr tee)
        : m_component(component)
        , m_tee(tee)
    {
    }

    void indent() noexcept override
    {
        auto& td = m_threadData.data();
        ++td.indent;
    }

    void unindent() noexcept override
    {
        auto& td = m_threadData.data();
        ErAssert(td.indent > 0);
        --td.indent;
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        m_tee->addSink(name, sink);
    }

    void removeSink(std::string_view name) override
    {
        m_tee->removeSink(name);
    }

    ISink::Ptr findSink(std::string_view name) override
    {
        return m_tee->findSink(name);
    }

protected:
    struct PerThread
    {
        unsigned indent = 0;
    };

    using ThreadDataHolder = ThreadData<PerThread>;
    std::string_view m_component;
    ITee::Ptr m_tee;
    ThreadDataHolder m_threadData;
};



} // namespace Er::Log::Private {}