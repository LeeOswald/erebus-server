
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
    assert.cpp
    condition.cpp
    exception.cpp
    flags.cpp
    inifile.cpp
    log.cpp
    lrucache.cpp
    main.cpp
    property.cpp
    propertybag.cpp
    stringutil.cpp
    tree.cpp
    ${PLATFORM_SOURCES}
)

target_link_libraries(${ER_TESTS} gtest_main ${EREBUS_LIB})

target_compile_features(${ER_TESTS} PUBLIC ${ER_CXX_FEATURES})

gtest_discover_tests(${ER_TESTS})
