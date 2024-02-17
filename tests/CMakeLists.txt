set(TARGET ertests)

enable_testing()

add_executable(
    ${TARGET}
    common.hpp
    exception.cpp
    flags.cpp
    log.cpp
    main.cpp
    stringutil.cpp
    tree.cpp
)

target_link_libraries(${TARGET} gtest_main ${EREBUS_LIB})

target_compile_features(${TARGET} PUBLIC ${ER_CXX_FEATURES})
