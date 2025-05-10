# Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_compile_definitions(BOOST_STACKTRACE_LINK)

if(ER_LINUX)
    add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
    set(Boost_LIBRARIES boost_filesystem boost_program_options boost_stacktrace_backtrace)
endif()

# {fmt}
find_package(fmt REQUIRED)

# GTest
find_package(GTest REQUIRED)

# lua
find_package(Lua REQUIRED)

# rapidjson
find_package(RapidJSON REQUIRED)

# protobuf
find_package(protobuf REQUIRED)

# gRPC
find_package(gRPC CONFIG REQUIRED)
