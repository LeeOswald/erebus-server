add_executable(${EREBUS_CTL}
    log.cxx
    log.hxx
    main.cxx
)


target_link_libraries(${EREBUS_CTL} PRIVATE Boost::program_options ${EREBUS_LIB} ${EREBUS_CLTLIB})

target_compile_features(${EREBUS_CTL} PUBLIC ${ER_CXX_FEATURES})
