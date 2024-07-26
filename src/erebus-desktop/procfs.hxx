#pragma once

#include <erebus/log.hxx>
#include <erebus-desktop/erebus-desktop.hxx>

#include <unordered_map>


namespace Erp
{

namespace Desktop
{



constexpr uint64_t InvalidPid = uint64_t(-1);
constexpr uint64_t KernelPid = 0;


class ProcFs final
    : public Er::NonCopyable
{
public:
    explicit ProcFs(const std::string& root, Er::Log::ILog* const log);

    std::optional<uint64_t> getUid(uint64_t pid) const noexcept;
    std::string readComm(uint64_t pid) const noexcept;
    std::string readExePath(uint64_t pid) const noexcept;
    std::unordered_map<std::string, std::string> readEnviron(uint64_t pid) const noexcept;

private:
    const std::string m_root;
    Er::Log::ILog* const m_log;
};



} // namespace Desktop {}

} // namespace Erp {}
