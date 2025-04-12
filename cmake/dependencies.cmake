# Boost
find_package(Boost REQUIRED)
add_compile_definitions(BOOST_STACKTRACE_LINK)
if(ER_POSIX)
    add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
endif()

# {fmt}
find_package(fmt REQUIRED)
# {fmt} wants this
if(MSVC)
    add_compile_options("/utf-8")
endif()

# GTest
find_package(GTest REQUIRED)

# lua
find_package(Lua REQUIRED)

# rapidjson
find_package(RapidJSON REQUIRED)

# valijson
find_package(valijson REQUIRED)

# protobuf
find_package(protobuf REQUIRED)