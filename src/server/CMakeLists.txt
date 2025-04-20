add_subdirectory(lib)
add_library(erebus::server_lib ALIAS erebus-server-lib)


add_executable(erebus-server
    main.cxx
    server_app.cxx
    server_app.hxx
)


target_link_libraries(erebus-server PRIVATE 
    fmt::fmt 
    ${Boost_LIBRARIES} 
    erebus::rtl 
    erebus::grpc_server
)

# help improve stacktraces
set_property(TARGET erebus-server PROPERTY ENABLE_EXPORTS ON)
