set(TARGET_NAME erebus-proctree-client)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
    PRIVATE
        ../protocol.cxx
        ../trace.hxx
        process_list_client.cxx

    PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/proctree/process_props.hxx
                ${ER_INCLUDE_DIR}/proctree/proctree.hxx
                ${ER_INCLUDE_DIR}/proctree/protocol.hxx
                ${ER_INCLUDE_DIR}/proctree/client/iprocess_list_client.hxx
)


target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    erebus::rtl_lib
    erebus::grpc_client_lib
)

target_compile_definitions(${TARGET_NAME} PRIVATE ER_PROCTREE_EXPORTS)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

