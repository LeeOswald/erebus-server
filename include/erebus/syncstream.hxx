#pragma once

#include <erebus/erebus.hxx>

#if defined(__GNUC__) && (__GNUC__ < 11)
    #include <mutex>
    #include <ostream>
    #include <sstream>

    namespace Er
    {
        class osyncstream
            : public std::ostringstream
        {
        public:
            ~osyncstream()
            {
                std::lock_guard l(mutex());
                m_stream << str();
            }

            explicit osyncstream(std::ostream& stream) noexcept
                : m_stream(stream)
            {}
            
        private:
            static std::mutex& mutex() noexcept
            {
                static std::mutex m;
                return m;
            }

            std::ostream& m_stream;
        };

    } // namespace Er {}

#else
    #include <syncstream>

    namespace Er
    {
        using osyncstream = std::osyncstream;

    } // namespace Er {}

#endif
