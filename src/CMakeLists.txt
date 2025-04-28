add_subdirectory(rtl-lib)
add_library(erebus::rtl_lib ALIAS erebus-rtl-lib)

add_subdirectory(test-lib)
add_library(erebus::test_lib ALIAS erebus-test-lib)

add_subdirectory(ipc)

add_subdirectory(server)

if (NOT ER_WINDOWS)
    add_subdirectory(proctree)
    add_library(erebus::proctree ALIAS erebus-proctree)
endif()
