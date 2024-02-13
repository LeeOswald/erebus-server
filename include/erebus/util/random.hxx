#pragma once

#include <erebus/erebus.hxx>

#include <random>
#include <sstream>


namespace Er
{

namespace Util
{

class Random final
    : public Er::NonCopyable
{
public:
    Random() noexcept
        : m_generator(device()())
    {
    }

    std::string generate(size_t length, std::string_view range) noexcept
    {
        std::uniform_int_distribution<> distrib(0, range.size() - 1);
        std::ostringstream ss;
        while (length--)
        {
            auto c = *(range.data() + distrib(m_generator));
            assert(c);
            ss << c;
        }

        return ss.str();
    }

private:
    static std::random_device& device()
    {
        static std::random_device d;
        return d;
    }

    std::mt19937 m_generator;
};

} // namespace Util {}

} // namespace Er {}