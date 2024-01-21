#pragma once

#include <erebus/erebus.hxx>

#if defined(__GNUC__) && (__GNUC__ < 11)
    #include <mutex>
    #include <ostream>
    #include <sstream>

    namespace Er
    {
        class osyncstream final
            : public boost::noncopyable
        {
        public:
            ~osyncstream()
            {
                std::lock_guard l(mutex());
                m_stream << m_ss.str();
            }

            explicit osyncstream(std::ostream& stream) noexcept
                : m_stream(stream)
            {}

            template <typename T>
            osyncstream& operator<<(T&& v)
            {
                m_ss << std::forward<T>(v);
                return *this;
            }

        private:
            static std::mutex& mutex() noexcept
            {
                static std::mutex m;
                return m;
            }

            std::ostream& m_stream;
            std::ostringstream m_ss;
        };

    } // namespace Er {}

#else
    #include <syncstream>

    namespace Er
    {
        using osyncstream = std::osyncstream;

    } // namespace Er {}

#endif
