set(PROTOBUF_GENERATED_DIR "${PROJECT_BINARY_DIR}/protobuf")
file(MAKE_DIRECTORY ${PROTOBUF_GENERATED_DIR})

set(ER_PROTO_DIR "${ER_INCLUDE_DIR}/ipc/grpc")

get_filename_component(PROPERTY_PROTO "${ER_PROTO_DIR}/property.proto" ABSOLUTE)
get_filename_component(EXCEPTION_PROTO "${ER_PROTO_DIR}/exception.proto" ABSOLUTE)

add_custom_command(
    OUTPUT "${PROTOBUF_GENERATED_DIR}/property.pb.cc" "${EREBUS_GENERATED_DIR}/property.pb.h"
    COMMAND ${PROTOC_PROGRAM}
    ARGS --cpp_out "${PROTOBUF_GENERATED_DIR}"
        -I "${ER_PROTO_DIR}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_PROGRAM}"
        "${PROPERTY_PROTO}"
    DEPENDS "${PROPERTY_PROTO}"
)

add_custom_command(
    OUTPUT "${PROTOBUF_GENERATED_DIR}/exception.pb.cc" "${EREBUS_GENERATED_DIR}/exception.pb.h"
    COMMAND ${PROTOC_PROGRAM}
    ARGS --cpp_out "${PROTOBUF_GENERATED_DIR}"
        -I "${ER_PROTO_DIR}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_PROGRAM}"
        "${EXCEPTION_PROTO}"
    DEPENDS "${EXCEPTION_PROTO}"
)

add_library(erebus-grpc-common STATIC
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_server.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_service.hxx
    ${EREBUS_GENERATED_DIR}/exception.pb.h
    ${EREBUS_GENERATED_DIR}/property.pb.h
    ${PROTOBUF_GENERATED_DIR}/exception.pb.cc
    ${PROTOBUF_GENERATED_DIR}/property.pb.cc
)


set_property(TARGET erebus-grpc-common PROPERTY PREFIX "")
set_property(TARGET erebus-grpc-common PROPERTY POSITION_INDEPENDENT_CODE ON)

set(GRPC_AND_DEPS
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    gRPC::gpr 
    gRPC::grpc 
    protobuf::libprotoc 
    protobuf::libprotobuf 
    absl::flags 
    absl::flags_parse 
    absl::log absl::log_internal_check_op 
)

if(NOT ER_WINDOWS)
    list(APPEND GRPC_AND_DEPS utf8_range::utf8_validity)
endif()

target_link_libraries(erebus-grpc-common PUBLIC ${GRPC_AND_DEPS} fmt::fmt erebus::rtl)

target_include_directories(erebus-grpc-common PUBLIC "${gRPC_INCLUDE_DIR}")
target_include_directories(erebus-grpc-common PUBLIC "${protobuf_INCLUDE_DIR}")
target_include_directories(erebus-grpc-common PUBLIC "${absl_INCLUDE_DIR}")

target_link_directories(erebus-grpc-common PUBLIC ${gRPC_INCLUDE_DIR}/../lib)




