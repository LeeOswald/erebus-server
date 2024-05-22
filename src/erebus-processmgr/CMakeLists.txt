add_library(${EREBUS_PROCESSMGR} SHARED
    ../../include/erebus-processmgr/processmgr.hxx
    ../../include/erebus-processmgr/processprops.hxx
    ../../include/erebus-processmgr/procfs.hxx
    iconcache.hxx
    iconcache.cxx
    iconmanager.hxx
    iconmanager.cxx
    processdetails.cxx
    processdetails.hxx
    processlist.cxx
    processlist.hxx
    processlistdiff.cxx
    processlistdiff.hxx
    processmgr.cxx
    procfs.cxx
)

add_compile_definitions(ER_PROCESSMGR_EXPORTS=1)

set_property(TARGET ${EREBUS_PROCESSMGR} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_PROCESSMGR} PRIVATE ${EREBUS_RTLLIB} ${EREBUS_DESKTOPLIB}) 
