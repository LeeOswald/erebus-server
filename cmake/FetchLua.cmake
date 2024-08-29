FetchContent_Declare(
    lua
    GIT_REPOSITORY "https://github.com/LeeOswald/lua-forge"
    GIT_TAG "master"
)

FetchContent_GetProperties(lua)

set(LUA_ENGINE "Lua")

if(NOT lua_POPULATED)
    FetchContent_Populate(lua)
endif()


add_subdirectory("${lua_SOURCE_DIR}" "${lua_BINARY_DIR}")
include_directories("${lua_SOURCE_DIR}/lua")


