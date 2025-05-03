#pragma once

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/system/thread.hxx>
#include <erebus/rtl/util/thread_data.hxx>
#include <erebus/rtl/util/unknown_base.hxx>



namespace Er::Log::Private
{

template <class UserThreadData>
    requires std::is_nothrow_default_constructible_v<UserThreadData>
class LoggerBase
    : public Util::ReferenceCountedBase<Util::ObjectBase<ILogger>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<ILogger>>;

public:
    ~LoggerBase() = default;

    LoggerBase(std::string_view component, TeePtr tee)
        : Base()
        , m_component(component)
        , m_tee(tee)
    {
    }

    void write(Level level, Time::ValueType time, uintptr_t tid, const std::string& message) override
    {
        if (level < m_level)
            return;

        auto indent = m_threadData.data().indent;
        write(makeRecord(m_component, level, time, tid, message, indent));
    }

    void write(Level level, Time::ValueType time, uintptr_t tid, std::string&& message) override
    {
        if (level < m_level)
            return;

        auto indent = m_threadData.data().indent;
        write(makeRecord(m_component, level, time, tid, std::move(message), indent));
    }

    void write(RecordPtr r) override
    {
        if (!r) [[unlikely]]
            return;

        if (r->level() < m_level)
            return;

        writeImpl(r);
    }

    void write(AtomicRecordPtr a) override
    {
        auto count = a->size();
        if (!count)
            return;

        std::vector<RecordPtr> filtered;
        filtered.reserve(count);

        for (decltype(count) i = 0; i < count; ++i)
        {
            auto r = a->get(i);

            if (!r)
                continue;

            if (r->level() < m_level)
                continue;

            filtered.push_back(r);
        }

        if (!filtered.empty())
            writeImpl(makeAtomicRecord(std::move(filtered)));
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
                doWrite(makeAtomicRecord(std::move(td.atomic)));
                
                ErAssert(td.atomic.empty());
            }
        }
    }

    void addSink(std::string_view name, SinkPtr sink) override
    {
        m_tee->addSink(name, sink);
    }

    void removeSink(std::string_view name) override
    {
        m_tee->removeSink(name);
    }

    SinkPtr findSink(std::string_view name) override
    {
        return m_tee->findSink(name);
    }

protected:
    virtual void doWrite(RecordPtr r) = 0;
    virtual void doWrite(AtomicRecordPtr a) = 0;

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

    void writeImpl(AtomicRecordPtr a)
    {
        auto& td = m_threadData.data();
        if (td.block > 0)
        {
            auto count = a->size();
            for (decltype(count) i = 0; i < count; ++i)
                td.atomic.push_back(a->get(i));
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
        std::vector<RecordPtr> atomic;
    };

    using ThreadDataHolder = ThreadData<PerThread>;
    std::string_view m_component;
    TeePtr m_tee;
    ThreadDataHolder m_threadData;
};



} // namespace Er::Log::Private {}