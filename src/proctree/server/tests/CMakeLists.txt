set(TARGET_NAME erebus-proctree-tests)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        procfs.cpp
    PRIVATE
        FILE_SET private_headers TYPE HEADERS
            FILES
                common.hpp
)

target_link_libraries(${TARGET_NAME} PRIVATE erebus::test_lib erebus::proctree erebus::rtl_lib)

add_test(NAME erebus-proctree COMMAND ${TARGET_NAME})


