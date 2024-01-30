add_executable(${EREBUS_SERVER}
    logger.cxx
    logger.hxx
    main.cxx
)


target_link_libraries(${EREBUS_SERVER} PRIVATE ${PLATFORM_LIBRARIES} Boost::filesystem Boost::program_options ${EREBUS_LIB} ${EREBUS_SRVLIB})

target_compile_features(${EREBUS_SERVER} PUBLIC ${ER_CXX_FEATURES})
