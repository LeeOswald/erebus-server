#pragma once

#include <erebus/log.hxx>

#include <erebus/luaxx/luaxx_exception_handler.hxx>
#include <erebus/luaxx/luaxx_registry.hxx>
#include <erebus/luaxx/luaxx_selector.hxx>


namespace Er::Lua 
{

class EREBUS_EXPORT State 
    : public Er::NonCopyable
{
public:
    ~State();

    explicit State(Er::Log::ILog* log, bool openLibs);
    explicit State(Er::Log::ILog* log, lua_State* l);
    
    
    Selector operator[](const char* name) const;
    bool operator()(const char* code);

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

    void openLib(const std::string& name, lua_CFunction openf);
    bool loadString(std::string_view str, const char* name = nullptr);
    bool load(const std::string& fileName);

    void setExceptionHandler(ExceptionHandler::function handler)
    {
        *m_exceptionHandler = ExceptionHandler(std::move(handler));
    }

protected:
    void exceptionHandler(int luaStatusCode, std::string msg, std::exception_ptr exception);

    Er::Log::ILog* m_log;
    lua_State* m_l;
    bool m_owner;
    std::unique_ptr<Er::Lua::Registry> m_registry;
    std::unique_ptr<Er::Lua::ExceptionHandler> m_exceptionHandler;
};

} // namespace Er::Lua {}