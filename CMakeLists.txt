cmake_minimum_required(VERSION 3.25)


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

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DER_64=1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DER_32=1)
else()
    message(FATAL_ERROR "Unsupported bitness")
endif()


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ER_BUILD_CLIENT_LIBS_ONLY "Build client libs only" OFF)
option(ER_ENABLE_ASSERT "Enable asserts even in release build" OFF)
option(ER_ENABLE_STACKTRACE "Enable stack traces for exceptions & assertions" ON)

if(ER_ENABLE_ASSERT)
    add_definitions(-DER_ENABLE_ASSERT=1)
endif()    

if(ER_ENABLE_STACKTRACE)
    add_definitions(-DER_ENABLE_STACKTRACE=1)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(ER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/erebus")
set(ER_PROTO_GENERATED_DIR "${PROJECT_BINARY_DIR}/protobuf")
file(MAKE_DIRECTORY ${ER_PROTO_GENERATED_DIR})
set(ER_PROTO_DIR "${ER_INCLUDE_DIR}/ipc/grpc")

include(debug)
include(dependencies)
include(protoc)

# detect OS
if(WIN32)
    set(ER_WINDOWS TRUE)
    add_definitions(-DER_WINDOWS=1)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX TRUE)
    set(ER_POSIX TRUE)
    add_definitions(-DER_LINUX=1)
    add_definitions(-DER_POSIX=1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

# -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/erebus-version.h.in" erebus-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Windows-specific settings
if(ER_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0602)
endif()

if(NOT ER_BUILD_CLIENT_LIBS_ONLY)
    include(CTest)
endif()

add_subdirectory(src)
add_subdirectory(tools)
