list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)


# Build libbpf
include(ExternalProject)

ExternalProject_Add(libbpf
  PREFIX libbpf
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd_party/libbpf/src
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
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd_party/bpftool/src
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
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd_party/blazesym
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

# for generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR})

bpf_object(sched sched.bpf.c)
add_dependencies(sched_skel libbpf-build bpftool-build generate_vmlinux_h)

add_library(${EREBUS_PROCMON} SHARED
    process_spy.cxx
    process_spy.hxx
    procmon.cxx
    procmon.hxx
)

add_compile_definitions(ER_PROCMON_EXPORTS=1)

target_compile_features(${EREBUS_PROCMON} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_PROCMON} PRIVATE sched_skel ${EREBUS_LIB} ${PLATFORM_LIBS}) 
