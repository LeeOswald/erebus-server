add_library(${EREBUS_CLTLIB} SHARED
    ../../include/erebus-clt/erebus-clt.hxx
    erebus-clt.cxx
)


add_compile_definitions(EREBUSCLT_EXPORTS=1)

target_compile_features(${EREBUS_CLTLIB} PUBLIC ${ER_CXX_FEATURES})

if(MSVC)
    target_link_libraries(${EREBUS_CLTLIB} PRIVATE ${EREBUS_LIB} ${EREBUS_PROTOLIB} ${PLATFORM_LIBS})
else()
    target_link_libraries(${EREBUS_CLTLIB} PRIVATE ${EREBUS_LIB} ${GRPC_DEPS} ${GRPC_LIBS} ${EREBUS_PROTOLIB} ${PLATFORM_LIBS})
endif()