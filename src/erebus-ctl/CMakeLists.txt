add_executable(${EREBUS_CTL}
    common.hxx
    main.cxx
)


target_link_libraries(${EREBUS_CTL} PRIVATE ${Boost_LIBRARIES} ${EREBUS_RTLLIB} ${EREBUS_CLTLIB})

# help improve stacktraces
set_property(TARGET ${EREBUS_CTL} PROPERTY ENABLE_EXPORTS ON)
