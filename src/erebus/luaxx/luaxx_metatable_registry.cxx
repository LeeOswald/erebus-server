#include <erebus/log.hxx>
#include <erebus/luaxx/luaxx_metatable_registry.hxx>

#include <shared_mutex>
#include <unordered_map>

namespace Er
{

class UserTypeRegistry final
    : public Er::NonCopyable
{
public:
    UserTypeRegistry(Er::Log::ILog* log)
        : m_log(log)
    {
    }

    const void* findOrAdd(const char* name)
    {
        std::string_view key(name);

        // maybe already there
        {
            std::shared_lock l(m_mutex);
            auto it = m_entries.find(key);
            if (it != m_entries.end())
                return it->second.get();
        }

        // add
        {
            std::unique_lock l(m_mutex);
            auto r = m_entries.insert({ key, std::make_unique<Entry>() });
            auto entry = r.first->second.get();
            if (r.second)
            {
                // really added
                Er::Log::debug(m_log, "Registered type [{}] -> {}", name, Format::ptr(entry));
            }

            return entry;
        }
    }

private:
    struct Entry
    {
        Entry() = default;

        using Ptr = std::unique_ptr<Entry>;
    };

    Er::Log::ILog* m_log;
    std::shared_mutex m_mutex;
    std::unordered_map<std::string_view, Entry::Ptr> m_entries;
};


static UserTypeRegistry* s_utr = nullptr;

EREBUS_EXPORT void initializeLua(Er::Log::ILog* log)
{
    ErAssert(!s_utr);
    s_utr = new UserTypeRegistry(log);
}

EREBUS_EXPORT void finalizeLua()
{
    if (s_utr)
    {
        delete s_utr;
        s_utr = nullptr;
    }
}

EREBUS_EXPORT const void* registerUserType(const char* name)
{
    ErAssert(s_utr);

    if (!s_utr)
        return nullptr;

    return s_utr->findOrAdd(name);
}

} // namespace Er {}


namespace Er::Lua
{

namespace MetatableRegistry
{

namespace detail
{

void _create_table_in_registry(lua_State* state, const std::string& name)
{
    lua_pushlstring(state, name.c_str(), name.size());
    lua_newtable(state);
    lua_settable(state, LUA_REGISTRYINDEX);
}

void _push_names_table(lua_State* state)
{
    lua_pushliteral(state, "erebus_metatable_names");
    lua_gettable(state, LUA_REGISTRYINDEX);
}

void _push_meta_table(lua_State* state)
{
    lua_pushliteral(state, "erebus_metatables");
    lua_gettable(state, LUA_REGISTRYINDEX);
}

void _push_typeinfo(lua_State* state, TypeID type)
{
    lua_pushlightuserdata(state, const_cast<void*>(type));
}

void _get_metatable(lua_State* state, TypeID type)
{
    detail::_push_meta_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);
    lua_remove(state, -2);
}

} // namespace detail {}


EREBUS_EXPORT void Create(lua_State* state)
{
    detail::_create_table_in_registry(state, "erebus_metatable_names");
    detail::_create_table_in_registry(state, "erebus_metatables");
}

EREBUS_EXPORT void PushNewMetatable(lua_State* state, TypeID type, const std::string& name)
{
    detail::_push_names_table(state);

    detail::_push_typeinfo(state, type);
    lua_pushlstring(state, name.c_str(), name.size());
    lua_settable(state, -3);

    lua_pop(state, 1);

    luaL_newmetatable(state, name.c_str()); // Actual result.

    detail::_push_meta_table(state);

    detail::_push_typeinfo(state, type);
    lua_pushvalue(state, -3);
    lua_settable(state, -3);

    lua_pop(state, 1);
}

EREBUS_EXPORT bool SetMetatable(lua_State* state, TypeID type)
{
    detail::_get_metatable(state, type);

    if (lua_istable(state, -1))
    {
        lua_setmetatable(state, -2);
        return true;
    }

    lua_pop(state, 1);
    return false;
}

EREBUS_EXPORT bool IsRegisteredType(lua_State* state, TypeID type)
{
    detail::_push_names_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);

    bool registered = lua_isstring(state, -1);
    lua_pop(state, 2);
    return registered;
}

EREBUS_EXPORT std::string GetTypeName(lua_State* state, TypeID type)
{
    std::string name("unregistered type");

    detail::_push_names_table(state);
    detail::_push_typeinfo(state, type);
    lua_gettable(state, -2);

    if (lua_isstring(state, -1))
    {
        size_t len = 0;
        char const * str = lua_tolstring(state, -1, &len);
        name.assign(str, len);
    }

    lua_pop(state, 2);
    return name;
}

EREBUS_EXPORT std::string GetTypeName(lua_State* state, int index)
{
    std::string name;

    if (lua_getmetatable(state, index))
    {
        lua_pushliteral(state, "__name");
        lua_gettable(state, -2);

        if (lua_isstring(state, -1))
        {
            size_t len = 0;
            char const * str = lua_tolstring(state, -1, &len);
            name.assign(str, len);
        }

        lua_pop(state, 2);
    }

    if (name.empty())
    {
        name = lua_typename(state, lua_type(state, index));
    }

    return name;
}

EREBUS_EXPORT bool IsType(lua_State* state, TypeID type, const int index)
{
    bool equal = true;

    if (lua_getmetatable(state, index))
    {
        detail::_get_metatable(state, type);
        equal = lua_istable(state, -1) && lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
    }
    else
    {
        detail::_get_metatable(state, type);
        equal = !lua_istable(state, -1);
        lua_pop(state, 1);
    }

    return equal;
}

EREBUS_EXPORT void CheckType(lua_State* state, TypeID type, const int index)
{
    if (!IsType(state, type, index))
    {
        throw Er::Lua::detail::GetUserdataParameterFromLuaTypeError(GetTypeName(state, type), index);
    }
}

} // namespace MetatableRegistry {}


} // namespace Er::Lua {}