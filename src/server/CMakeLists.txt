add_subdirectory(lib)
add_library(erebus::server_lib ALIAS erebus-server-lib)

set(TARGET_NAME erebus-server)

add_executable(${TARGET_NAME}
    main.cxx
    server_app.cxx
    server_app.hxx
)


target_link_libraries(${TARGET_NAME} PRIVATE 
    fmt::fmt 
    ${Boost_LIBRARIES} 
    erebus::rtl_lib 
    erebus::grpc_server_lib
)

# help improve stacktraces
set_property(TARGET ${TARGET_NAME} PROPERTY ENABLE_EXPORTS ON)
