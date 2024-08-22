#pragma once

#include <erebus/log.hxx>

#include <erebus/luaxx/luaxx_exception_handler.hxx>
#include <erebus/luaxx/luaxx_registry.hxx>
#include <erebus/luaxx/luaxx_selector.hxx>


namespace Er
{

class EREBUS_EXPORT LuaState final
    : public Er::NonCopyable
{
public:
    ~LuaState();
    explicit LuaState(Er::Log::ILog* log);

    Er::Lua::Selector operator[](const char* name) const
    {
        return Er::Lua::Selector(m_l, *m_registry, *m_exceptionHandler, name);
    }

    bool operator()(const char* code)
    {
        Er::Lua::ResetStackOnScopeExit savedStack(m_l);
        int status = luaL_dostring(m_l, code);
        if (status)
        {
            m_exceptionHandler->Handle_top_of_stack(status, m_l);
            return false;
        }

        return true;
    }

    void forceGC()
    {
        lua_gc(m_l, LUA_GCCOLLECT, 0);
    }

    uint64_t memUsed()
    {
        uint32_t kbytes = lua_gc(m_l, LUA_GCCOUNT, 0);
        uint32_t bytes = lua_gc(m_l, LUA_GCCOUNTB, 0);
        return uint64_t(kbytes) * 1024 + bytes;
    }

    int size() const
    {
        return lua_gettop(m_l);
    }

    bool loadFromString(std::string_view str, const char* name = nullptr);

private:
    void exceptionHandler(int luaStatusCode, std::string msg, std::exception_ptr exception);

    Er::Log::ILog* m_log;
    lua_State* m_l;
    std::unique_ptr<Er::Lua::Registry> m_registry;
    std::unique_ptr<Er::Lua::ExceptionHandler> m_exceptionHandler;
};


} // namespace Er {}
