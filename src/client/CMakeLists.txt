add_executable(erebus-client
    client_app.cxx
    client_app.hxx
    main.cxx
    runner_base.hxx
    ping.hxx
    system_info.hxx
)


target_link_libraries(erebus-client PRIVATE 
    fmt::fmt 
    ${Boost_LIBRARIES} 
    erebus::rtl 
    erebus::grpc_client
)

# help improve stacktraces
set_property(TARGET erebus-client PROPERTY ENABLE_EXPORTS ON)