
include(GoogleTest)

set(EREBUS_RTL_TESTS erebus-rtl-tests)


if(ER_LINUX)
    set(PLATFORM_SOURCES 
        sighandler.cpp
    )
endif()


enable_testing()

add_executable(
    ${EREBUS_RTL_TESTS}
    common.hpp
    condition.cpp
    exception.cpp
    flags.cpp
    inifile.cpp
    log.cpp
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

target_link_libraries(${EREBUS_RTL_TESTS} gtest_main ${EREBUS_RTLLIB} ${LUA_LIBRARIES})

gtest_discover_tests(${EREBUS_RTL_TESTS})
