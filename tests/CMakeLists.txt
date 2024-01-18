set(TARGET ertests)

enable_testing()

add_executable(
    ${TARGET}
    common.hpp
    main.cpp
)

target_link_libraries(${TARGET} gtest_main ${EREBUS_LIB})

target_compile_features(${TARGET} PUBLIC ${ER_CXX_FEATURES})
