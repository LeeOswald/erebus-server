function(generate_protobuf_stuff BASE_FILE_NAME IN_DIR OUT_DIR)

    get_filename_component(PROTO_PATH "${IN_DIR}/${BASE_FILE_NAME}.proto" ABSOLUTE)

    add_custom_command(
        OUTPUT "${OUT_DIR}/${BASE_FILE_NAME}.pb.cc" "${OUT_DIR}/${BASE_FILE_NAME}.pb.h"
        COMMAND ${PROTOC_PROGRAM}
        ARGS --cpp_out "${OUT_DIR}"
            -I "${IN_DIR}"
            "${PROTO_PATH}"
        DEPENDS "${PROTO_PATH}"
    )

endfunction() 

function(generate_grpc_stuff BASE_FILE_NAME IN_DIR OUT_DIR)

    get_filename_component(PROTO_PATH "${IN_DIR}/${BASE_FILE_NAME}.proto" ABSOLUTE)

    add_custom_command(
        OUTPUT "${OUT_DIR}/${BASE_FILE_NAME}.pb.cc" "${OUT_DIR}/${BASE_FILE_NAME}.pb.h" "${OUT_DIR}/${BASE_FILE_NAME}.grpc.pb.cc" "${OUT_DIR}/${BASE_FILE_NAME}.grpc.pb.h"
        COMMAND ${PROTOC_PROGRAM}
        ARGS --grpc_out "${OUT_DIR}" 
            --cpp_out "${OUT_DIR}"
            -I "${IN_DIR}"
            --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_PROGRAM}"
            "${PROTO_PATH}"
        DEPENDS "${PROTO_PATH}"
    )

endfunction()
