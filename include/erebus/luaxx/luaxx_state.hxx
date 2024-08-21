#pragma once

#include <erebus/luaxx/luaxx_exception_handler.hxx>
#include <erebus/luaxx/luaxx_registry.hxx>
#include <erebus/luaxx/luaxx_selector.hxx>
#include <erebus/luaxx/luaxx_util.hxx>

#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Er::Lua 
{

class EREBUS_EXPORT State 
{
private:
    lua_State* _l;
    bool _l_owner;
    std::unique_ptr<Registry> _registry;
    std::unique_ptr<ExceptionHandler> _exception_handler;

public:
    State();

    State(bool should_open_libs);
    State(lua_State* l);

    State(const State& other) = delete;
    State& operator=(const State& other) = delete;

    State(State&& other);
    State& operator=(State&& other);
    
    ~State();

    int Size() const;

    bool LoadFromFile(const std::string& file);
    bool LoadFromString(std::string_view str, const char* name = nullptr);
    void OpenLib(const std::string& modname, lua_CFunction openf);

    void HandleExceptionsPrintingToStdOut();
    void HandleExceptionsWith(ExceptionHandler::function handler);

    Selector operator[](const char* name) const;
    bool operator()(const char* code);

    void ForceGC();
    uint64_t GetMemUsed();

    friend std::ostream& operator<<(std::ostream& os, const State& state);
};


inline std::ostream& operator<<(std::ostream& os, const State& state) 
{
    os << "Er::Lua::State - " << state._l;
    return os;
}

} // namespace Er::Lua {}
