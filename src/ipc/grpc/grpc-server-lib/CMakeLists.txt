set(TARGET_NAME erebus-grpc-server-lib)

add_library(${TARGET_NAME} SHARED
    grpc_server.cxx
    system_info_service.cxx
    trace.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/server/grpc_server.hxx 
    ${ER_INCLUDE_DIR}/ipc/grpc/server/iserver.hxx 
    ${ER_INCLUDE_DIR}/ipc/grpc/server/iservice.hxx 
)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

target_link_libraries(${TARGET_NAME} PUBLIC erebus::grpc_common_lib erebus::server_lib fmt::fmt erebus::rtl_lib)
