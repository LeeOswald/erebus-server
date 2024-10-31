
add_library(${EREBUS_SRVLIB} SHARED
    ../../include/erebus-srv/erebus-srv.hxx
    ../../include/erebus-srv/global_requests.hxx
    ../../include/erebus-srv/plugin.hxx
    erebus-srv.cxx
    erebus_service.hxx
    erebus_service.cxx
)


add_compile_definitions(EREBUSSRV_EXPORTS=1)

set_property(TARGET ${EREBUS_SRVLIB} PROPERTY PREFIX "")

target_precompile_headers(${EREBUS_SRVLIB} PRIVATE ../../include/erebus/erebus.hxx)

target_link_libraries(${EREBUS_SRVLIB} PRIVATE ${EREBUS_RTLLIB} ${GRPC_LIBS} ${EREBUS_PROTOLIB})
