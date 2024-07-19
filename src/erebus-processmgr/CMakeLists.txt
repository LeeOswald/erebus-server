add_library(${EREBUS_PROCESSMGR} SHARED
    ../../include/erebus-processmgr/erebus-processmgr.hxx
    processdetailsservice.cxx
    processdetailsservice.hxx
    processlistcollector.cxx
    processlistcollector.hxx
    processlistdiff.cxx
    processlistdiff.hxx
    processlistservice.cxx
    processlistservice.hxx
    processmgrplugin.cxx
    procfs.cxx
    procfs.hxx
)

add_compile_definitions(ER_PROCESSMGR_EXPORTS=1)

set_property(TARGET ${EREBUS_PROCESSMGR} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_PROCESSMGR} PRIVATE ${EREBUS_RTLLIB}) 
