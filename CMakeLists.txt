cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(version)

get_version_from_file("version.txt")

project(erebus-server VERSION ${ER_VERSION})

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE isn't defined. Debug value will be used by default.")
    set(CMAKE_BUILD_TYPE Debug)
else()
    if(${CMAKE_BUILD_TYPE} MATCHES "^[Rr][Ee][Ll][Ee][Aa][Ss][Ee]$")
        set(CMAKE_BUILD_TYPE Release)
    elseif(${CMAKE_BUILD_TYPE} MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
        set(CMAKE_BUILD_TYPE Debug)
    elseif(${CMAKE_BUILD_TYPE} MATCHES "^[Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo]$")
        set(CMAKE_BUILD_TYPE RelWithDebInfo)    
    else()
        message(WARNING "CMAKE_BUILD_TYPE isn't Release or Debug. Debug value will be used by default.")
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# help debugging concepts
if(NOT MSVC)
    add_compile_options(-fconcepts-diagnostics-depth=2)
endif() 

# -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# -fvisibility=hidden
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT MSVC)
    add_link_options(-rdynamic) # enable stack capture
endif()

set(ER_LINUX 0)
set(ER_WINDOWS 0)
set(ER_POSIX 0)

set(CMAKE_CXX_STANDARD 20)
set(ER_CXX_FEATURES cxx_std_20)

if(WIN32)
    set(ER_WINDOWS 1)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX 1)
    set(ER_POSIX 1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()


# require gRPC & protobuf
if(NOT MSVC)
    find_package(utf8_range REQUIRED)
endif()
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/erebus-version.h.in erebus-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    ${PROJECT_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)


set(EREBUS_PROTOLIB erebus-proto)

# fix poorly linked gRPC dependencies on linux
if(NOT MSVC)
    set(GRPC_DEPS
        absl::flags 
        absl::flags_parse 
        absl::log absl::log_internal_check_op 
        utf8_range::utf8_validity
    )
endif()

set(GRPC_LIBS
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    gRPC::gpr 
    gRPC::grpc 
    protobuf::libprotoc 
    protobuf::libprotobuf 
)

# require Boost
if(MSVC)
    find_package(Boost REQUIRED COMPONENTS filesystem iostreams system program_options)
else()
    find_package(Boost REQUIRED COMPONENTS 
        filesystem
        iostreams
        stacktrace_basic
        stacktrace_backtrace
        stacktrace_addr2line
        stacktrace_noop 
        system 
        program_options
    )
endif()


include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(WIN32)
    # disable autolinking in boost
    add_definitions(-DBOOST_ALL_NO_LIB)
    # force all boost libraries to dynamic link
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# force shared CRT
if(MSVC)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()


# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()


# component names
set(EREBUS_LIB erebus-rtl)
set(EREBUS_PROCESSMGR processmgr)
set(EREBUS_PROCMON procmon)
set(EREBUS_SRVLIB erebus-srv)
set(EREBUS_CLTLIB erebus-clt)
set(EREBUS_SERVER erebus-server)
set(EREBUS_CTL erebus-ctl)

# components
add_subdirectory(protocol)
add_subdirectory(src/erebus)
if(ER_LINUX)
    add_subdirectory(src/erebus-processmgr)
    add_subdirectory(src/erebus-procmon)
endif()
add_subdirectory(src/erebus-srv)
add_subdirectory(src/erebus-clt)
add_subdirectory(src/erebus-server)
add_subdirectory(src/erebus-ctl)

# don't build tests if we're a subproject
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
