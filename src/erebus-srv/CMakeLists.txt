
add_library(${EREBUS_SRVLIB} SHARED
    ../../include/erebus-srv/erebus-srv.hxx
    ../../include/erebus-srv/plugin.hxx
    auth.hxx
    rpc.hxx
    svcbase.hxx
    erebus.cxx
    erebus-srv.cxx
    svcbase.cxx
)


add_compile_definitions(EREBUSSRV_EXPORTS=1)

target_compile_features(${EREBUS_SRVLIB} PUBLIC ${ER_CXX_FEATURES})

if(MSVC)
    target_link_libraries(${EREBUS_SRVLIB} PRIVATE ${EREBUS_LIB} ${EREBUS_PROTOLIB} ${PLATFORM_LIBS})
else()
    target_link_libraries(${EREBUS_SRVLIB} PRIVATE ${EREBUS_LIB} ${GRPC_DEPS} ${GRPC_LIBS} ${EREBUS_PROTOLIB} ${PLATFORM_LIBS})
endif()
