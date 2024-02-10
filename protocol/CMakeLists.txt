# Generated from .proto

get_filename_component(COMMON_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/common.proto" ABSOLUTE)
get_filename_component(EREBUS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/erebus.proto" ABSOLUTE)
get_filename_component(EREBUS_PROTO_PATH "${EREBUS_PROTO}" DIRECTORY)

set(COMMON_PB_CC "${EREBUS_PROTO_PATH}/erebus/common.pb.cc")
set(COMMON_PB_H "${EREBUS_PROTO_PATH}/erebus/common.pb.h")
set(COMMON_GRPC_PB_CC "${EREBUS_PROTO_PATH}/erebus/common.grpc.pb.cc")
set(COMMON_GRPC_PB_H "${EREBUS_PROTO_PATH}/erebus/common.grpc.pb.h")


set(EREBUS_PB_CC "${EREBUS_PROTO_PATH}/erebus/erebus.pb.cc")
set(EREBUS_PB_H "${EREBUS_PROTO_PATH}/erebus/erebus.pb.h")
set(EREBUS_GRPC_PB_CC "${EREBUS_PROTO_PATH}/erebus/erebus.grpc.pb.cc")
set(EREBUS_GRPC_PB_H "${EREBUS_PROTO_PATH}/erebus/erebus.grpc.pb.h")

if(ER_WINDOWS)
    set(GRPC_PLUGIN "${gRPC_DIR}/../../tools/grpc/grpc_cpp_plugin.exe")
else()
    set(GRPC_PLUGIN "${gRPC_DIR}/../../../bin/grpc_cpp_plugin")
endif()

add_custom_command(
    OUTPUT "${COMMON_PB_CC}" "${COMMON_PB_H}" "${COMMON_GRPC_PB_CC}" "${COMMON_GRPC_PB_H}"
    COMMAND protoc
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        -I "${EREBUS_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_PLUGIN}"
        "${COMMON_PROTO}"
    DEPENDS "${COMMON_PROTO}"
)


add_custom_command(
    OUTPUT "${EREBUS_PB_CC}" "${EREBUS_PB_H}" "${EREBUS_GRPC_PB_CC}" "${EREBUS_GRPC_PB_H}"
    COMMAND protoc
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        -I "${EREBUS_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_PLUGIN}"
        "${EREBUS_PROTO}"
    DEPENDS "${EREBUS_PROTO}"
)


add_library(${EREBUS_PROTOLIB} STATIC
    ${COMMON_PB_CC}
    ${COMMON_PB_H}
    ${COMMON_GRPC_PB_CC}
    ${COMMON_GRPC_PB_H}
    ${EREBUS_PB_CC}
    ${EREBUS_PB_H}
    ${EREBUS_GRPC_PB_CC}
    ${EREBUS_GRPC_PB_H}
)

target_link_libraries(${EREBUS_PROTOLIB} ${GRPC_DEPS} ${GRPC_LIBS})
