add_library(${EREBUS_DESKTOPLIB} SHARED
    ../../include/erebus-desktop/erebus-desktop.hxx
    ../../include/erebus-desktop/ic.hxx
    erebus-desktop.cxx
    desktopfilecache.cxx
    desktopfilecache.hxx
    iconcacheipc.cxx
    iconcache.hxx
    iconcache.cxx
    iconresolver.hxx
    iconresolver.cxx
    procfs.hxx
    procfs.cxx
    service.hxx
    service.cxx
    util.cxx
)


add_compile_definitions(EREBUSDESKTOP_EXPORTS=1)

set_property(TARGET ${EREBUS_DESKTOPLIB} PROPERTY PREFIX "")

target_precompile_headers(${EREBUS_DESKTOPLIB} PRIVATE ../../include/erebus/erebus.hxx)

target_link_libraries(${EREBUS_DESKTOPLIB} PRIVATE ${Boost_LIBRARIES} ${EREBUS_RTLLIB})
