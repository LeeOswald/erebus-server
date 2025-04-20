add_library(erebus-grpc-client STATIC
    channel.cxx
    system_info_client.cxx
    trace.hxx
    ${ER_INCLUDE_DIR}/ipc/grpc/grpc_client.hxx 
    ${ER_INCLUDE_DIR}/ipc/grpc/system_info_client.hxx 
)

set_property(TARGET erebus-grpc-client PROPERTY PREFIX "")

target_link_libraries(erebus-grpc-client PUBLIC erebus::grpc_common fmt::fmt erebus::rtl)