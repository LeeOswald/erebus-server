cmake_minimum_required(VERSION 3.20)

function(get_version_from_file VER_FILE_NAME)
    file(READ ${VER_FILE_NAME} ER_VERSION_RAW)
    # Remove trailing whitespaces and/or newline
    string(STRIP ${ER_VERSION_RAW} ER_VERSION_)
    set(ER_VERSION ${ER_VERSION_} CACHE STRING
        "Project version determined from version.txt" FORCE
    )
    message(STATUS "Determined project version ${ER_VERSION}")
endfunction()


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
    else()
        message(WARNING "CMAKE_BUILD_TYPE isn't Release or Debug. Debug value will be used by default.")
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

if(MSVC)
else()
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/erebus-version.h.in erebus-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ER_LINUX)
    # require pthreads
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBRARIES Threads::Threads)
endif()

# require Boost
find_package(Boost REQUIRED COMPONENTS system program_options)

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

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# require gRPC & protobuf
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# component names
set(EREBUS_LIB erebus-rtl)
set(EREBUS_SERVER erebus-server)
set(EREBUS_CTL erebus-ctl)

# components
add_subdirectory(3rd_party/googletest)
add_subdirectory(src/erebus)
add_subdirectory(src/erebus-server)
add_subdirectory(tests)
