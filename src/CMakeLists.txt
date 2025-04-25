add_subdirectory(rtl)
add_library(erebus::rtl ALIAS erebus-rtl)

add_subdirectory(testing)
add_library(erebus::testing ALIAS erebus-testing)

add_subdirectory(ipc)

add_subdirectory(server)

add_subdirectory(client)

if (NOT ER_WINDOWS)
    add_subdirectory(proctree)
    add_library(erebus::proctree ALIAS erebus-proctree)
endif()
