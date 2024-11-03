add_executable(${EREBUS_CTL}
    common.hxx
    main.cxx
    request_runner.hxx
    stream_runner.hxx
)


target_link_libraries(${EREBUS_CTL} PRIVATE ${Boost_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_CLTLIB})

target_precompile_headers(${EREBUS_CTL} PRIVATE ../../include/erebus/erebus.hxx)

# help improve stacktraces
set_property(TARGET ${EREBUS_CTL} PROPERTY ENABLE_EXPORTS ON)
