
add_library(${EREBUS_SRVLIB} SHARED
    ../../include/erebus-srv/erebus-srv.hxx
    ../../include/erebus-srv/plugin.hxx
    rpc.hxx
    svcbase.hxx
    erebus.cxx
    erebus-srv.cxx
    svcbase.cxx
)


add_compile_definitions(EREBUSSRV_EXPORTS=1)

target_link_libraries(${EREBUS_SRVLIB} PRIVATE ${EREBUS_RTLLIB} ${GRPC_LIBS} ${EREBUS_PROTOLIB})
