add_library(${EREBUS_CLTLIB} SHARED
    ../../include/erebus-clt/erebus-clt.hxx
    erebus-clt.cxx
)


add_compile_definitions(EREBUSCLT_EXPORTS=1)

set_property(TARGET ${EREBUS_CLTLIB} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_CLTLIB} PRIVATE ${EREBUS_RTLLIB} ${GRPC_LIBS} ${EREBUS_PROTOLIB})
