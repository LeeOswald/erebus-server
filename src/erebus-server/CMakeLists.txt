add_executable(${EREBUS_SERVER}
    config.cxx
    config.hxx
    coreservice.cxx
    coreservice.hxx
    main.cxx
    pluginmgr.cxx
    pluginmgr.hxx
)


target_link_libraries(${EREBUS_SERVER} PRIVATE ${Boost_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_SRVLIB})

target_precompile_headers(${EREBUS_SERVER} PRIVATE ../../include/erebus/erebus.hxx)

# help improve stacktraces
set_property(TARGET ${EREBUS_SERVER} PROPERTY ENABLE_EXPORTS ON)
