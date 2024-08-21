#include <erebus/erebus.hxx>
#include <erebus/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

State::~State()
{
    if (_l != nullptr && _l_owner)
    {
        ForceGC();
        lua_close(_l);
    }

    _l = nullptr;
}

State::State()
    : State(false)
{}

State::State(bool should_open_libs)
    : _l(nullptr)
    , _l_owner(true)
    , _exception_handler(new ExceptionHandler)
{
    _l = luaL_newstate();
    if (_l == nullptr)
        throw 0;

    if (should_open_libs)
        luaL_openlibs(_l);

    _registry.reset(new Registry(_l));
    HandleExceptionsPrintingToStdOut();
}

State::State(lua_State* l)
    : _l(l)
    , _l_owner(false)
    , _exception_handler(new ExceptionHandler)
{
    _registry.reset(new Registry(_l));
    HandleExceptionsPrintingToStdOut();
}

State::State(State&& other)
    : _l(other._l),
    _l_owner(other._l_owner),
    _registry(std::move(other._registry))
{
    other._l = nullptr;
}

State& State::operator=(State&& other)
{
    if (&other == this)
        return *this;

    _l = other._l;
    _l_owner = other._l_owner;
    _registry = std::move(other._registry);
    other._l = nullptr;

    return *this;
}

bool State::LoadFromFile(const std::string& file)
{
    ResetStackOnScopeExit savedStack(_l);
    int status = luaL_loadfile(_l, file.c_str());
#if LUA_VERSION_NUM >= 502
    auto const lua_ok = LUA_OK;
#else
    auto const lua_ok = 0;
#endif
    if (status != lua_ok)
    {
        if (status == LUA_ERRSYNTAX)
        {
            const char* msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : file + ": syntax error");
        }
        else if (status == LUA_ERRFILE)
        {
            const char* msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : file + ": file error");
        }

        return false;
    }

    status = lua_pcall(_l, 0, LUA_MULTRET, 0);
    if (status == lua_ok)
    {
        return true;
    }

    const char* msg = lua_tostring(_l, -1);
    _exception_handler->Handle(status, msg ? msg : file + ": dofile failed");
    return false;
}

bool State::LoadFromString(std::string_view str, const char* name)
{
    ResetStackOnScopeExit savedStack(_l);
    int status = luaL_loadbuffer(_l, str.data(), str.length(), name);
#if LUA_VERSION_NUM >= 502
    auto const lua_ok = LUA_OK;
#else
    auto const lua_ok = 0;
#endif
    if (status != lua_ok)
    {
        if (status == LUA_ERRSYNTAX)
        {
            const char* msg = lua_tostring(_l, -1);
            _exception_handler->Handle(status, msg ? msg : "luaL_loadstring() failed");
        }

        return false;
    }

    status = lua_pcall(_l, 0, LUA_MULTRET, 0);
    if (status == lua_ok)
    {
        return true;
    }

    const char* msg = lua_tostring(_l, -1);
    _exception_handler->Handle(status, msg ? msg : "lua_pcall() failed");

    return false;
}

void State::OpenLib(const std::string& modname, lua_CFunction openf)
{
    ResetStackOnScopeExit savedStack(_l);
#if LUA_VERSION_NUM >= 502
    luaL_requiref(_l, modname.c_str(), openf, 1);
#else
    lua_pushcfunction(_l, openf);
    lua_pushstring(_l, modname.c_str());
    lua_call(_l, 1, 0);
#endif
}

void State::HandleExceptionsPrintingToStdOut()
{
    *_exception_handler = ExceptionHandler([](int, std::string msg, std::exception_ptr) {_print(msg); });
}

void State::HandleExceptionsWith(ExceptionHandler::function handler)
{
    *_exception_handler = ExceptionHandler(std::move(handler));
}

Selector State::operator[](const char* name) const
{
    return Selector(_l, *_registry, *_exception_handler, name);
}

bool State::operator()(const char* code)
{
    ResetStackOnScopeExit savedStack(_l);
    int status = luaL_dostring(_l, code);
    if (status)
    {
        _exception_handler->Handle_top_of_stack(status, _l);
        return false;
    }

    return true;
}

void State::ForceGC()
{
    lua_gc(_l, LUA_GCCOLLECT, 0);
}

uint64_t State::GetMemUsed()
{
    uint32_t kbytes = lua_gc(_l, LUA_GCCOUNT, 0);
    uint32_t bytes = lua_gc(_l, LUA_GCCOUNTB, 0);
    return uint64_t(kbytes) * 1024 + bytes;
}

int State::Size() const
{
    return lua_gettop(_l);
}


} // namespace Er::Lua {}