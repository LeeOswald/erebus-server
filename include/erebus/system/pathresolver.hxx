#pragma once

#include <erebus/erebus.hxx>


#include <vector>

namespace Er
{
    
namespace System
{

class EREBUS_EXPORT PathResolver final
    : public Er::NonCopyable
{
public:
    explicit PathResolver(const char* paths = nullptr);

    std::optional<std::string> resolve(std::string_view name) const;
    
private:
    std::vector<std::string> m_paths;
};

} // namespace System {}

} // namespace Er {}
