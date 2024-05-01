#include "common.hpp"

#include <erebus/log.hxx>

#include <mutex>
#include <thread>

namespace
{

class Logger
    : public Er::Log::LogBase
{
public:
    explicit Logger(Er::Log::Level level)
        : Er::Log::LogBase(level, 65536)
    {
        Er::Log::LogBase::addDelegate("this", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
        Er::Log::LogBase::unmute();
    }

    std::queue<std::shared_ptr<Er::Log::Record>>& queue()
    {
        return m_queue;
    }

private:
    void delegate(std::shared_ptr<Er::Log::Record> r)
    {
        std::lock_guard l(m_mutex);

        m_queue.push(r);
    }

    std::mutex m_mutex;
    std::queue<std::shared_ptr<Er::Log::Record>> m_queue;
};

} // namespace {}


TEST(Er_LogBase, simple)
{
    Logger log(Er::Log::Level::Warning);

    // this record will be dropped
    log.write(Er::Log::Level::Info, Er::Log::Location(),  "hello world");

    log.write(Er::Log::Level::Warning, Er::Log::Location(), "simple warning");
    
    // this record will be dropped
    log.write(Er::Log::Level::Debug, Er::Log::Location(), "hello world");
    
    // test formatting
    log.write(Er::Log::Level::Error, Er::Log::Location(), "format %s %d", "test", 12);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto& q = log.queue();

    ASSERT_EQ(q.size(), 2);
    auto r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Warning);
    EXPECT_STREQ(r->message.c_str(), "simple warning");

    q.pop();

    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Error);
    EXPECT_STREQ(r->message.c_str(), "format test 12");

}

TEST(Er_LogWrapper, simple)
{
    Logger log(Er::Log::Level::Info);

    // char
    Er::Log::Warning(&log, ErLogNowhere()) << 'f' << 'a' << 'c' << 't';
    // const char*
    Er::Log::Warning(&log, ErLogNowhere()) << "hello " << "world";
    // std::string_view
    Er::Log::Warning(&log, ErLogNowhere()) << std::string_view("string_") << std::string_view("view");
    // std::string
    Er::Log::Warning(&log, ErLogNowhere()) << std::string("std::") << std::string("string");
    // bool
    Er::Log::Error(&log, ErLogNowhere()) << std::boolalpha << true << false;
    // const void*
#if ER_64
    auto pvoid = reinterpret_cast<const void*>(uintptr_t(0xBABAEBA0FFFFFFFF));
#else
    auto pvoid = reinterpret_cast<const void*>(uintptr_t(0xBABAEBA0));
#endif
    Er::Log::Error(&log, ErLogNowhere()) << pvoid;
    // int16_t
    Er::Log::Info(&log, ErLogNowhere()) << int16_t(10) << std::hex << std::setw(4) << std::setfill('0') << int16_t(0xad) << int16_t(0x2020);
    // uint16_t
    Er::Log::Info(&log, ErLogNowhere()) << uint16_t(10) << std::hex << std::setw(4) << std::setfill('0') << uint16_t(0xad) << uint16_t(0x2020);
    // int32_t
    Er::Log::Info(&log, ErLogNowhere()) << int32_t(333) << std::hex << std::setw(8) << std::setfill('0') << int32_t(0xfdad) << int32_t(0xdeaa2020);
    // uint32_t
    Er::Log::Info(&log, ErLogNowhere()) << uint32_t(333) << std::hex << std::setw(8) << std::setfill('0') << uint32_t(0xfdad) << uint32_t(0xdeaa2020);
    // int64_t
    Er::Log::Info(&log, ErLogNowhere()) << int64_t(4444) << std::hex << std::setw(16) << std::setfill('0') << int64_t(0x77778888) << int64_t(0x7fffffff55664455);
    // uint64_t
    Er::Log::Info(&log, ErLogNowhere()) << uint64_t(4444) << std::hex << std::setw(16) << std::setfill('0') << uint64_t(0x77778888) << uint64_t(0x7fffffff55664455);
    // float
    Er::Log::Info(&log, ErLogNowhere()) << std::fixed << std::setprecision(3) << float(0.1233);
    // double
    Er::Log::Info(&log, ErLogNowhere()) << std::fixed << std::setprecision(3) << double(0.1233);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto& q = log.queue();

    ASSERT_EQ(q.size(), 14);
    auto r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Warning);
    EXPECT_STREQ(r->message.c_str(), "fact");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Warning);
    EXPECT_STREQ(r->message.c_str(), "hello world");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Warning);
    EXPECT_STREQ(r->message.c_str(), "string_view");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Warning);
    EXPECT_STREQ(r->message.c_str(), "std::string");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Error);
    EXPECT_STREQ(r->message.c_str(), "truefalse");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Error);
    if (r->message.starts_with("0x"))
        r->message.erase(0, 2);
        
#if ER_64
    EXPECT_STRCASEEQ(r->message.c_str(), "BABAEBA0FFFFFFFF");
#else
    EXPECT_STRCASEEQ(r->message.c_str(), "BABAEBA0");
#endif

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "1000ad2020");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "1000ad2020");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "3330000fdaddeaa2020");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "3330000fdaddeaa2020");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "444400000000777788887fffffff55664455");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STRCASEEQ(r->message.c_str(), "444400000000777788887fffffff55664455");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STREQ(r->message.c_str(), "0.123");

    q.pop();
    r = q.front();

    EXPECT_EQ(r->level, Er::Log::Level::Info);
    EXPECT_STREQ(r->message.c_str(), "0.123");

}