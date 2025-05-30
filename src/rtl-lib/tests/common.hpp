#pragma once

#include <gtest/gtest.h>

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include <iostream>
#include <mutex>
#include <sstream>


struct InstanceCounter
{
    static int instances;

    InstanceCounter()
    {
        ++instances;
    }

    InstanceCounter(const InstanceCounter&)
    {
        ++instances;
    }

    InstanceCounter& operator=(const InstanceCounter&)
    {
        ++instances;
        return *this;
    }

    ~InstanceCounter()
    {
        --instances;
    }
};

class CapturedStderr
    : public Er::Util::ReferenceCountedBase<Er::Util::ObjectBase<Er::Log::ISink>>
{
    using Base = Er::Util::ReferenceCountedBase<Er::Util::ObjectBase<Er::Log::ISink>>;

public:
    ~CapturedStderr() = default;

    CapturedStderr() noexcept
        : Base()
    {
    }
    
    void write(Er::Log::RecordPtr r) override
    {
        std::lock_guard l(m_mutex);
        m_out << r->message() << "\n";
    }

    void write(Er::Log::AtomicRecordPtr a) override
    {
        std::lock_guard l(m_mutex);

        auto& recs = a->get();
        for (auto& r : recs)
            m_out << r->message() << "\n";
    }

    bool flush(std::chrono::milliseconds) override
    {
        m_out.flush();
        return true;
    }

    std::string grab()
    {
        std::lock_guard l(m_mutex);
        auto s = m_out.str();
        std::stringstream().swap(m_out);
        return s;
    }

private:
    std::mutex m_mutex;
    std::stringstream m_out;
};