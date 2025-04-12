add_executable(erebus-server
    config.cxx
    config.hxx
    main.cxx
    server_app.cxx
    server_app.hxx
)


target_link_libraries(erebus-server PRIVATE fmt::fmt ${Boost_LIBRARIES} ValiJSON::valijson rapidjson erebus::rtl)

# help improve stacktraces
set_property(TARGET erebus-server PROPERTY ENABLE_EXPORTS ON)
