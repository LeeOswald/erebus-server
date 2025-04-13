set(PROTOBUF_GENERATED_DIR "${PROJECT_BINARY_DIR}/protobuf")
file(MAKE_DIRECTORY ${PROTOBUF_GENERATED_DIR})

set(ER_PROTO_DIR "${ER_INCLUDE_DIR}/ipc/grpc")

function(generate_protobuf_stuff BASE_FILE_NAME IN_DIR OUT_DIR)
    get_filename_component(PROTO_PATH "${IN_DIR}/${BASE_FILE_NAME}.proto" ABSOLUTE)

    add_custom_command(
        OUTPUT "${OUT_DIR}/${BASE_FILE_NAME}.pb.cc" "${OUT_DIR}/${BASE_FILE_NAME}.pb.h"
        COMMAND ${PROTOC_PROGRAM}
        ARGS --cpp_out "${OUT_DIR}"
            -I "${IN_DIR}"
            --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_PROGRAM}"
            "${PROTO_PATH}"
        DEPENDS "${PROTO_PATH}"
    )
endfunction() 

generate_protobuf_stuff("property" ${ER_PROTO_DIR} ${PROTOBUF_GENERATED_DIR})
generate_protobuf_stuff("exception" ${ER_PROTO_DIR} ${PROTOBUF_GENERATED_DIR})

add_library(erebus-grpc-common STATIC
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_server.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/igrpc_service.hxx
    ${PROTOBUF_GENERATED_DIR}/exception.pb.h
    ${PROTOBUF_GENERATED_DIR}/property.pb.h
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




