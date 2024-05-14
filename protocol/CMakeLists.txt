if(CMAKE_CROSSCOMPILING)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    find_program(PROTOC protoc)
else()
    set(GRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)
    set(PROTOC $<TARGET_FILE:protoc>)
endif()


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

add_custom_command(
    OUTPUT "${COMMON_PB_CC}" "${COMMON_PB_H}" "${COMMON_GRPC_PB_CC}" "${COMMON_GRPC_PB_H}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        -I "${EREBUS_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        "${COMMON_PROTO}"
    DEPENDS "${COMMON_PROTO}"
)


add_custom_command(
    OUTPUT "${EREBUS_PB_CC}" "${EREBUS_PB_H}" "${EREBUS_GRPC_PB_CC}" "${EREBUS_GRPC_PB_H}"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}/erebus/"
        -I "${EREBUS_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        "${EREBUS_PROTO}"
    DEPENDS "${EREBUS_PROTO}"
)


add_library(${EREBUS_PROTOLIB} STATIC
    erebus/protocol.hxx
    ${COMMON_PB_CC}
    ${COMMON_PB_H}
    ${COMMON_GRPC_PB_CC}
    ${COMMON_GRPC_PB_H}
    ${EREBUS_PB_CC}
    ${EREBUS_PB_H}
    ${EREBUS_GRPC_PB_CC}
    ${EREBUS_GRPC_PB_H}
)

set_property(TARGET ${EREBUS_PROTOLIB} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${EREBUS_PROTOLIB} PRIVATE ${GRPC_LIBS})

