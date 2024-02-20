set(TARGET ertests)

if(ER_LINUX)
    set(PLATFORM_SOURCES 
        sighandler.cpp
    )
endif()


enable_testing()

add_executable(
    ${TARGET}
    common.hpp
    condition.cpp
    exception.cpp
    flags.cpp
    log.cpp
    main.cpp
    stringutil.cpp
    tree.cpp
    ${PLATFORM_SOURCES}
)

target_link_libraries(${TARGET} gtest_main ${EREBUS_LIB})

target_compile_features(${TARGET} PUBLIC ${ER_CXX_FEATURES})
