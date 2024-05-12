#include <erebus/util/stringutil.hxx>
#include "pathresolver.hxx"

#include <filesystem>

namespace Er
{
    
namespace Private
{

PathResolver::PathResolver(const char* paths)
{
    std::string src = paths ? paths : std::getenv("PATH");
    ErAssert(!src.empty());
    m_paths = Er::Util::split(src, std::string_view(":"), Er::Util::SplitSkipEmptyParts);
}

std::optional<std::string> PathResolver::resolve(std::string_view name) const
{
    for (auto& path: m_paths)
    {
        std::filesystem::path p(path);
        auto dir = path;
        p.append(name);

        std::error_code ec;
        if (!std::filesystem::exists(p, ec) || ec)
            continue;

        if (std::filesystem::is_symlink(p, ec) && !ec)
        {
            auto target = std::filesystem::read_symlink(p, ec);
            if (!ec)
            {
                if (target.is_relative())
                {
                    auto tmp = dir / target;
                    target = std::filesystem::canonical(tmp, ec);
                    if (ec)
                        return std::nullopt;
                }

                if (std::filesystem::exists(target, ec) && !ec)
                    return std::make_optional(target.string());
            }
            else
            {
                return std::nullopt;
            }
        }

        return std::make_optional(p.string());
    }

    return std::nullopt;
}


} // namespace Private {}

} // namespace Er {}
