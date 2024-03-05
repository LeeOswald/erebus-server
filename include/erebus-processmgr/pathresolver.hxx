#pragma once

#include <erebus-processmgr/processmgr.hxx>


#include <vector>

namespace Er
{

class ER_PROCESSMGR_EXPORT PathResolver final
    : public Er::NonCopyable
{
public:
    explicit PathResolver(const char* paths = nullptr);

    std::optional<std::string> resolve(std::string_view name) const;
    
private:
    std::vector<std::string> m_paths;
};

} // namespace Er {}