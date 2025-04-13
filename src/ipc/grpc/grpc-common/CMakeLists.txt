
generate_protobuf_stuff("common" ${ER_PROTO_DIR} ${ER_PROTO_GENERATED_DIR})

add_library(erebus-grpc-common STATIC
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_server.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_service.hxx
    ${ER_PROTO_GENERATED_DIR}/common.pb.h
    ${ER_PROTO_GENERATED_DIR}/common.pb.cc
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




