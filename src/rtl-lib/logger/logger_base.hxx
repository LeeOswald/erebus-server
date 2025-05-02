#pragma once

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/system/thread.hxx>
#include <erebus/rtl/util/thread_data.hxx>

#include <boost/noncopyable.hpp>


namespace Er::Log::Private
{

template <class UserThreadData>
    requires std::is_nothrow_default_constructible_v<UserThreadData>
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

    void write(RecordPtr r) override
    {
        if (!r) [[unlikely]]
            return;

        if (r->level() < m_level)
            return;

        auto indent = m_threadData.data().indent;
        if (indent > 0)
            r->setIndent(r->indent() + indent);

        if (!m_component.empty() && r->component().empty())
            r->setComponent(m_component);

        writeImpl(r);
    }

    void write(AtomicRecord a) override
    {
        if (a.empty())
            return;

        AtomicRecord filtered;
        filtered.reserve(a.size());

        auto indent = m_threadData.data().indent;

        for (auto r : a)
        {
            if (!r)
                continue;

            if (r->level() < m_level)
                continue;

            if (indent > 0)
                r->setIndent(r->indent() + indent);

            if (!m_component.empty() && r->component().empty())
                r->setComponent(m_component);

            filtered.push_back(r);
        }

        if (!a.empty())
            writeImpl(a);
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

    void beginBlock() noexcept override
    {
        auto& td = m_threadData.data();
        ++td.block;
    }

    void endBlock() noexcept override
    {
        auto& td = m_threadData.data();
        ErAssert(td.block > 0);
        if (--td.block == 0)
        {
            if (!td.atomic.empty())
            {
                doWrite(td.atomic);
                td.atomic.clear();
            }
        }
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
    virtual void doWrite(RecordPtr r) = 0;
    virtual void doWrite(AtomicRecord a) = 0;

    void writeImpl(RecordPtr r)
    {
        auto& td = m_threadData.data();
        if (td.block > 0)
        {
            td.atomic.push_back(r);
        }
        else
        {
            doWrite(r);
        }
    }

    void writeImpl(AtomicRecord a)
    {
        auto& td = m_threadData.data();
        if (td.block > 0)
        {
            for (auto& r: a)
                td.atomic.push_back(r);
        }
        else
        {
            doWrite(a);
        }
    }

    struct PerThread
        : public UserThreadData
    {
        unsigned indent = 0;
        unsigned block = 0;
        AtomicRecord atomic;
    };

    using ThreadDataHolder = ThreadData<PerThread>;
    std::string_view m_component;
    ITee::Ptr m_tee;
    ThreadDataHolder m_threadData;
};



} // namespace Er::Log::Private {}