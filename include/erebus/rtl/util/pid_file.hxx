#pragma once

#include <erebus/rtl/system/process.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/lock_file.hxx>



namespace Er::Util
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


} // namespace Er::Util {}

