add_library(${EREBUS_DESKTOPLIB} SHARED
    ../../include/erebus-desktop/appentry.hxx
    ../../include/erebus-desktop/erebus-desktop.hxx
    ../../include/erebus-desktop/ic.hxx
    ../../include/erebus-desktop/protocol.hxx
    appentrymonitor.cxx
    erebus-desktop.cxx
    iconcacheipc.cxx
    iconcache.hxx
    iconcache.cxx
    iconresolver.hxx
    iconresolver.cxx
    service.hxx
    service.cxx
    util.cxx
)


add_compile_definitions(EREBUSDESKTOP_EXPORTS=1)

set_property(TARGET ${EREBUS_DESKTOPLIB} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_DESKTOPLIB} PRIVATE ${Boost_LIBRARIES} ${EREBUS_RTLLIB})
