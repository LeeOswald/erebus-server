
#include <erebus/rtl/type_id.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Er
{

namespace
{

class UserTypeRegistry final
    : public boost::noncopyable
{
public:
    UserTypeRegistry() = default;

    RegisteredType* findOrAdd(std::string_view key)
    {
        // maybe already there
        {
            std::shared_lock l(m_mutex);
            auto it = m_entries.find(key);
            if (it != m_entries.end())
                return &it->second;
        }

        // add
        {
            std::unique_lock l(m_mutex);
            auto id = m_next++;
            auto r = m_entries.insert({ key, RegisteredType(key, id) });
            auto entry = &r.first->second;

            return entry;
        }
    }

    RegisteredType const* find(TypeIndex index) const noexcept
    {
        std::shared_lock l(m_mutex);
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [index](decltype(m_entries)::value_type const& e) { return e.second.index == index; });
        if (it == m_entries.end())
            return nullptr;

        return &(it->second);
    }

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string_view, RegisteredType> m_entries;
    TypeIndex m_next = 0;
};

UserTypeRegistry& registry()
{
    static UserTypeRegistry utr;
    return utr;
}

} // namespace {}


ER_RTL_EXPORT RegisteredType const* lookupType(std::string_view name)
{
    return registry().findOrAdd(name);
}

ER_RTL_EXPORT RegisteredType const* lookupType(TypeIndex index)
{
    return registry().find(index);
}

} // namespace Er {}