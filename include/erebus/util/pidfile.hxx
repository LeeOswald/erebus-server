#pragma once

#include <erebus/system/process.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/lockfile.hxx>



namespace Er
{

namespace Util
{

class PidFile final 
    : public LockFile
{
public:
    explicit PidFile(const std::string& path)
        : LockFile(path)
    {
        put(std::to_string(Er::System::CurrentProcess::id()));
    }

    static std::optional<Er::System::Pid> read(const std::string& path) noexcept
    {
        try
        {
            auto str = loadTextFile(path);
            return std::strtoull(str.c_str(), nullptr, 10);
        }
        catch (...)
        {
        }

        return std::nullopt;
    }

};


} // namespace Util {}

} // namespace Er {}

