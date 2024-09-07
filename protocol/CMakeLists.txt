if(ER_USE_SYSTEM_GRPC)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    find_program(PROTOC protoc)
else()
    set(GRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)
    set(PROTOC $<TARGET_FILE:protoc>)
endif()


set(EREBUS_GENERATED_DIR ${PROJECT_BINARY_DIR}/erebus)
file(MAKE_DIRECTORY ${EREBUS_GENERATED_DIR})


get_filename_component(EREBUS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/erebus.proto" ABSOLUTE)

add_custom_command(
    OUTPUT "${EREBUS_GENERATED_DIR}/erebus.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.pb.h" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.cc" "${EREBUS_GENERATED_DIR}/erebus.grpc.pb.h"
    COMMAND ${PROTOC}
    ARGS --grpc_out "${EREBUS_GENERATED_DIR}"
        --cpp_out "${EREBUS_GENERATED_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        "${EREBUS_PROTO}"
    DEPENDS "${EREBUS_PROTO}"
)

add_library(${EREBUS_PROTOLIB} STATIC
    erebus/protocol.hxx
    protocol.cxx
    ${EREBUS_GENERATED_DIR}/erebus.pb.cc 
    ${EREBUS_GENERATED_DIR}/erebus.pb.h 
    ${EREBUS_GENERATED_DIR}/erebus.grpc.pb.cc 
    ${EREBUS_GENERATED_DIR}/erebus.grpc.pb.h
)

set_property(TARGET ${EREBUS_PROTOLIB} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${EREBUS_PROTOLIB} PRIVATE ${GRPC_LIBS})

