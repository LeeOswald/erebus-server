set(TARGET_NAME erebus-grpc-client-lib)

add_library(${TARGET_NAME} STATIC
    channel.cxx
    system_info_client.cxx
    trace.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/client/grpc_client.hxx 
    ${ER_INCLUDE_DIR}/ipc/grpc/client/iclient.hxx 
    ${ER_INCLUDE_DIR}/ipc/grpc/client/isystem_info_client.hxx 
)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

target_link_libraries(${TARGET_NAME} PUBLIC erebus::grpc_common_lib fmt::fmt erebus::rtl_lib)