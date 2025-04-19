add_library(erebus-grpc-server STATIC
    grpc_server.cxx
    system_info_service.cxx
    trace.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/private/grpc_server.hxx 
)

set_property(TARGET erebus-grpc-server PROPERTY PREFIX "")

target_link_libraries(erebus-grpc-server PUBLIC erebus::grpc_common erebus::server_lib fmt::fmt erebus::rtl)
