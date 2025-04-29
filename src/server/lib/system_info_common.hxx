#pragma once


#include <erebus/server/system_info.hxx>

#include <mutex>
#include <shared_mutex>


namespace Er
{

namespace SystemInfo
{

struct Sources;

void registerSources(Sources&);


struct Sources
{
    std::shared_mutex mutex;
    std::map<std::string_view, Source> map;

    Sources()
    {
        registerSources(*this);
    }

    static Sources& instance()
    {
        static Sources s;
        return s;
    }
};

} // namespace SystemInfo {}

} // namespace Er {}