#pragma once

#include <gtest/gtest.h>


#include <erebus/rtl/log.hxx>

#include <iostream>
#include <mutex>


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
    : public Er::Log::ISink
{
public:
    CapturedStderr() = default;
    ~CapturedStderr() = default;

    void write(Er::Log::RecordPtr r) override
    {
        std::lock_guard l(m_mutex);
        m_out << r->message() << "\n";
    }

    void write(Er::Log::AtomicRecord a) override
    {
        std::lock_guard l(m_mutex);

        for (auto r : a)
        {
            m_out << r->message() << "\n";
        }
    }

    void flush() override
    {
        m_out.flush();
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