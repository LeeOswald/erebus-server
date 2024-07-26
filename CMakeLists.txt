cmake_minimum_required(VERSION 3.20)


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build configuration specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(version)
get_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/version.txt" ER_VERSION_)
set(ER_VERSION ${ER_VERSION_} CACHE STRING "Erebus server version determined from version file" FORCE)
message(STATUS "Determined Erebus server version ${ER_VERSION}")

project(erebus-server VERSION ${ER_VERSION})

option(ER_USE_SYSTEM_GRPC "Use system-provided gRPC" OFF)

include(ExternalProject)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/a")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


if(WIN32)
    set(ER_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX TRUE)
    set(ER_POSIX TRUE)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

if(ER_LINUX AND NOT ER_BUILD_CLIENT_LIBS_ONLY)
    include(bpf)
endif()

include(FetchLua)

include(FetchGRPC)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/erebus-version.h.in" erebus-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party" 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/protocol"
)


# require Boost
find_package(Boost REQUIRED COMPONENTS filesystem iostreams system program_options)


include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(ER_WINDOWS)
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
if(ER_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()


# component names
set(EREBUS_PROTOLIB erebus-proto)
set(EREBUS_RTLLIB erebus-rtl)
set(EREBUS_DESKTOPLIB erebus-desktop)
set(EREBUS_PROCESSMGR erebus-processmgr)
set(EREBUS_PROCMON erebus-procmon)
set(EREBUS_SRVLIB erebus-srv)
set(EREBUS_CLTLIB erebus-clt)
set(EREBUS_SERVER erebus-server)
set(EREBUS_CTL erebus-ctl)


# components
add_subdirectory(src/erebus)
if(ER_LINUX)
    add_subdirectory(src/erebus-desktop)
endif()
add_subdirectory(protocol)
add_subdirectory(src/erebus-srv)
add_subdirectory(src/erebus-clt)


if(NOT ER_BUILD_CLIENT_LIBS_ONLY)
    if(ER_LINUX)
        add_subdirectory(src/erebus-processmgr)
        add_subdirectory(src/erebus-procmon)
    endif()
    add_subdirectory(src/erebus-server)
    add_subdirectory(src/erebus-ctl)
endif()

if(NOT ER_BUILD_CLIENT_LIBS_ONLY)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
