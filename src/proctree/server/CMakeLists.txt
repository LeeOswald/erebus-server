set(TARGET_NAME erebus-proctree)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
    PRIVATE
        ../protocol.cxx
        linux/process_props_collector.cxx
        linux/process_props_collector.hxx
        linux/procfs.cxx
        plugin.cxx
        proctree_service.cxx
        proctree_service.hxx
        trace.hxx

    PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/proctree/process_props.hxx
                ${ER_INCLUDE_DIR}/proctree/protocol.hxx
                ${ER_INCLUDE_DIR}/proctree/server/linux/procfs.hxx
                ${ER_INCLUDE_DIR}/proctree/server/proctree.hxx
)


target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    erebus::rtl_lib
    erebus::grpc_server_lib
)

target_compile_definitions(${TARGET_NAME} PRIVATE ER_PROCTREE_EXPORTS)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

add_subdirectory(tests)
