add_subdirectory(lib)
add_library(erebus::server_lib ALIAS erebus-server-lib)



generate_grpc_stuff("system_info" ${ER_PROTO_DIR} ${ER_PROTO_GENERATED_DIR})

add_executable(erebus-server
    config.cxx
    config.hxx
    main.cxx
    server_app.cxx
    server_app.hxx
    ${ER_PROTO_GENERATED_DIR}/system_info.pb.h
    ${ER_PROTO_GENERATED_DIR}/system_info.pb.cc
    ${ER_PROTO_GENERATED_DIR}/system_info.grpc.pb.h
    ${ER_PROTO_GENERATED_DIR}/system_info.grpc.pb.cc
)


target_link_libraries(erebus-server PRIVATE 
    fmt::fmt 
    ${Boost_LIBRARIES} 
    ValiJSON::valijson 
    rapidjson 
    erebus::rtl 
    erebus::grpc_common
)

# help improve stacktraces
set_property(TARGET erebus-server PROPERTY ENABLE_EXPORTS ON)
