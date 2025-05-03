#include <erebus/rtl/logger/simple_filter.hxx>
#include <erebus/rtl/util/unknown_base.hxx>


namespace Er::Log
{

namespace
{

class LevelFilter
    : public Util::ReferenceCountedBase<Util::ObjectBase<IFilter>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<IFilter>>;

public:
    ~LevelFilter() = default;

    explicit LevelFilter(Level min, Level max) noexcept
        : m_minLevel(min)
        , m_maxLevel(max)
    {
    }

    bool filter(const IRecord* r) const noexcept override
    {
        auto l = r->level();
        return (l >= m_minLevel) && (l <= m_maxLevel);
    }

private:
    Level m_minLevel;
    Level m_maxLevel;
};


} // namespace {}


ER_RTL_EXPORT FilterPtr makeLevelFilter(Level min, Level max)
{
    return FilterPtr{ new LevelFilter(min, max) };
}

} // namespace Er::Log {}