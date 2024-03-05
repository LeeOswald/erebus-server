#include <erebus/util/stringutil.hxx>
#include <erebus-processmgr/pathresolver.hxx>

#include <filesystem>

namespace Er
{

PathResolver::PathResolver(const char* paths)
{
    std::string src = paths ? paths : std::getenv("PATH");
    assert(!src.empty());
    m_paths = Er::Util::split(src, std::string_view(":"), Er::Util::SplitSkipEmptyParts);
}

std::optional<std::string> PathResolver::resolve(std::string_view name) const
{
    for (auto& path: m_paths)
    {
        std::filesystem::path p(path);
        p.append(name);

        std::error_code ec;
        if (!std::filesystem::exists(p, ec))
            continue;

        return std::make_optional(p.string());
    }

    return std::nullopt;
}


} // namespace Er {}