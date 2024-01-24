set(GENERATED_SOURCES
    ../../protocol/erebus.grpc.pb.h
    ../../protocol/erebus.pb.h
    ../../protocol/erebus.grpc.pb.cc
    ../../protocol/erebus.pb.cc
)

add_library(${EREBUS_CLTLIB} SHARED
    ../../include/erebus-clt/erebus-clt.hxx
    erebus-clt.cxx
    ${GENERATED_SOURCES}
)


add_compile_definitions(EREBUSCLT_EXPORTS=1)

target_compile_features(${EREBUS_CLTLIB} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_CLTLIB} PUBLIC ${EREBUS_LIB} gRPC::grpc++ gRPC::grpc++_reflection gRPC::gpr gRPC::grpc gRPC::grpc++ protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite absl::flags absl::flags_parse absl::log absl::log_internal_check_op utf8_range::utf8_validity)