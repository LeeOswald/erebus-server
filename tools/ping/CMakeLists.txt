set(TARGET_NAME erebus-ping)

add_executable(${TARGET_NAME}
    client_app.cxx
    client_app.hxx
    main.cxx
    runner_base.hxx
    ping.hxx
    system_info.hxx
)


target_link_libraries(${TARGET_NAME} PRIVATE 
    fmt::fmt 
    ${Boost_LIBRARIES} 
    erebus::rtl_lib 
    erebus::grpc_client_lib
)

# help improve stacktraces
set_property(TARGET ${TARGET_NAME} PROPERTY ENABLE_EXPORTS ON)