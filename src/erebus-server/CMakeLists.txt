add_executable(${EREBUS_SERVER}
    config.cxx
    config.hxx
    logger.cxx
    logger.hxx
    main.cxx
    pluginmgr.cxx
    pluginmgr.hxx
    users.cxx
    users.hxx
)


target_link_libraries(${EREBUS_SERVER} PRIVATE ${PLATFORM_LIBRARIES} Boost::filesystem Boost::program_options ${EREBUS_LIB} ${EREBUS_SRVLIB} ${PLATFORM_LIBS})

target_compile_features(${EREBUS_SERVER} PUBLIC ${ER_CXX_FEATURES})
