add_library(${EREBUS_DESKTOPLIB} SHARED
    ../../include/erebus-desktop/erebus-desktop.hxx
    ../../include/erebus-desktop/desktopentries.hxx
    desktopentries.cxx
)


add_compile_definitions(EREBUSDESKTOP_EXPORTS=1)

set_property(TARGET ${EREBUS_DESKTOPLIB} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_DESKTOPLIB} PRIVATE ${EREBUS_RTLLIB})
