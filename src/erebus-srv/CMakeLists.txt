
add_library(${EREBUS_SRVLIB} SHARED
    ../../include/erebus-srv/erebus-srv.hxx
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
    target_link_libraries(${EREBUS_SRVLIB} PUBLIC ${EREBUS_LIB} PRIVATE ${EREBUS_PROTOLIB})
else()
    target_link_libraries(${EREBUS_SRVLIB} PUBLIC ${EREBUS_LIB} PRIVATE ${GRPC_DEPS} ${GRPC_LIBS} ${EREBUS_PROTOLIB})
endif()