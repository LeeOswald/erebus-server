
add_executable(erebus-proctree-tests)

target_sources(erebus-proctree-tests
    PRIVATE
        main.cpp
        procfs.cpp
    PRIVATE
        FILE_SET private_headers TYPE HEADERS
            FILES
                common.hpp
)

target_link_libraries(erebus-proctree-tests PRIVATE erebus::testing erebus::proctree erebus::rtl)

add_test(NAME erebus-proctree COMMAND erebus-proctree-tests)


