cmake_minimum_required(VERSION 3.20)

include(ExternalProject)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(version)
get_version_from_file("version.txt")

project(erebus-server VERSION ${ER_VERSION})


set(CMAKE_CXX_STANDARD 20)
set(ER_CXX_FEATURES cxx_std_20)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

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



if(WIN32)
    set(ER_WINDOWS 1)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX 1)
    set(ER_POSIX 1)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

if(ER_LINUX)
    # Build libbpf

    ExternalProject_Add(libbpf
        PREFIX libbpf
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libbpf/src
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make
            OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
            DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
            INCLUDEDIR=
            LIBDIR=
            UAPIDIR=
            install install_uapi_headers
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        STEP_TARGETS build
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.so
    )

    # Build bpftool
    ExternalProject_Add(bpftool
        PREFIX bpftool
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/bpftool/src
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make bootstrap
            OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        STEP_TARGETS build
    )

    find_program(CARGO_EXISTS cargo)
    if(CARGO_EXISTS)
    ExternalProject_Add(blazesym
        PREFIX blazesym
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/blazesym
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cargo build --verbose --package=blazesym-c --release
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        STEP_TARGETS build
    )
    endif()

    set(BPFOBJECT_BPFTOOL_EXE ${CMAKE_CURRENT_BINARY_DIR}/bpftool/bootstrap/bpftool)
    set(BPFOBJECT_VMLINUX_H ${CMAKE_CURRENT_BINARY_DIR}/vmlinux.h)
    set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)
    set(LIBBPF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.so)
    find_package(BpfObject REQUIRED)

    add_custom_command(
        OUTPUT ${BPFOBJECT_VMLINUX_H}
        COMMAND ${BPFOBJECT_BPFTOOL_EXE} btf dump file /sys/kernel/btf/vmlinux format c > ${BPFOBJECT_VMLINUX_H}
        DEPENDS bpftool-build
        VERBATIM
        COMMENT "Generating vmlinux.h"
    )

    add_custom_target(generate_vmlinux_h DEPENDS "${BPFOBJECT_VMLINUX_H}")

endif()


include(FetchGRPC)


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
