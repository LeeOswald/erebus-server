# Build libbpf
ExternalProject_Add(libbpf
    PREFIX libbpf
    GIT_REPOSITORY "https://github.com/libbpf/libbpf.git"
    GIT_TAG "master"
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/libbpf_src/src
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cd ./src && make
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
    GIT_REPOSITORY "https://github.com/libbpf/bpftool.git"
    GIT_TAG "main"
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bpftool_src/src
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cd ./src && make bootstrap
        OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
    STEP_TARGETS build
)

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
