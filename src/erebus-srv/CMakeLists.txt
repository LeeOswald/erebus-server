set(GENERATED_SOURCES
    ../../protocol/erebus.grpc.pb.h
    ../../protocol/erebus.pb.h
    ../../protocol/erebus.grpc.pb.cc
    ../../protocol/erebus.pb.cc
)

add_library(${EREBUS_SRVLIB} SHARED
    ../../include/erebus-srv/erebus-srv.hxx
    ../../include/erebus-srv/userdb.hxx
    erebus-srv.cxx
    ${GENERATED_SOURCES}
)


add_compile_definitions(EREBUSSRV_EXPORTS=1)

target_compile_features(${EREBUS_SRVLIB} PUBLIC ${ER_CXX_FEATURES})

# fix poorly linked gRPC dependencies on linux
if(NOT MSVC)
    set(GRPC_DEPS
        absl::flags 
        absl::flags_parse 
        absl::log absl::log_internal_check_op 
        utf8_range::utf8_validity
    )
endif()


target_link_libraries(${EREBUS_SRVLIB} PUBLIC ${EREBUS_LIB} PRIVATE gRPC::grpc++ gRPC::grpc++_reflection gRPC::gpr gRPC::grpc gRPC::grpc++ protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite ${GRPC_DEPS})