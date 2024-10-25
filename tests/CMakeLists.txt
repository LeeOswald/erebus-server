
include(FetchGTest)
include(GoogleTest)

set(ER_TESTS ertests)


if(ER_LINUX)
    set(PLATFORM_SOURCES 
        sighandler.cpp
    )
endif()


enable_testing()

add_executable(
    ${ER_TESTS}
    common.hpp
    condition.cpp
    exception.cpp
    flags.cpp
    inifile.cpp
    lrucache.cpp
    luaxx_class.cpp
    luaxx_error.cpp
    luaxx_exception.cpp
    luaxx_int64.cpp
    luaxx_interop.cpp
    luaxx_metatable.cpp
    luaxx_obj.cpp
    luaxx_property.cpp
    luaxx_reference.cpp
    luaxx_selector.cpp
    main.cpp
    property.cpp
    propertybag.cpp
    stringutil.cpp
    tree.cpp
    variant.cpp
    ${PLATFORM_SOURCES}
)

target_link_libraries(${ER_TESTS} gtest_main ${EREBUS_RTLLIB} ${LUA_LIBRARIES})

gtest_discover_tests(${ER_TESTS})
