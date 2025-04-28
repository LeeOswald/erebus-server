set(TARGET_NAME erebus-grpc-common-lib)

generate_protobuf_stuff("common" ${ER_PROTO_DIR} ${ER_PROTO_GENERATED_DIR})
generate_grpc_stuff("system_info" ${ER_PROTO_DIR} ${ER_PROTO_GENERATED_DIR})
generate_grpc_stuff("proctree" ${ER_PROTO_DIR} ${ER_PROTO_GENERATED_DIR})

add_library(${TARGET_NAME} STATIC
    ${ER_INCLUDE_DIR}/ipc/grpc/protocol.hxx
    ${ER_PROTO_GENERATED_DIR}/common.pb.h
    ${ER_PROTO_GENERATED_DIR}/common.pb.cc
    ${ER_PROTO_GENERATED_DIR}/system_info.pb.h
    ${ER_PROTO_GENERATED_DIR}/system_info.pb.cc
    ${ER_PROTO_GENERATED_DIR}/system_info.grpc.pb.h
    ${ER_PROTO_GENERATED_DIR}/system_info.grpc.pb.cc
    ${ER_PROTO_GENERATED_DIR}/proctree.pb.h
    ${ER_PROTO_GENERATED_DIR}/proctree.pb.cc
    ${ER_PROTO_GENERATED_DIR}/proctree.grpc.pb.h
    ${ER_PROTO_GENERATED_DIR}/proctree.grpc.pb.cc
    protocol.cxx
)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

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

target_link_libraries(${TARGET_NAME} PUBLIC ${GRPC_AND_DEPS} fmt::fmt erebus::rtl_lib)

target_include_directories(${TARGET_NAME} PUBLIC "${gRPC_INCLUDE_DIR}")
target_include_directories(${TARGET_NAME} PUBLIC "${protobuf_INCLUDE_DIR}")
target_include_directories(${TARGET_NAME} PUBLIC "${absl_INCLUDE_DIR}")

target_link_directories(${TARGET_NAME} PUBLIC ${gRPC_INCLUDE_DIR}/../lib)

