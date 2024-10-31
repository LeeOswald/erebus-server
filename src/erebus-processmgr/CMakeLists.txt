add_library(${EREBUS_PROCESSMGR} SHARED
    ../../include/erebus-processmgr/erebus-processmgr.hxx
    globalscollector.cxx
    globalscollector.hxx
    processdetailsservice.cxx
    processdetailsservice.hxx
    processlistcollector.cxx
    processlistcollector.hxx
    processlistservice.cxx
    processlistservice.hxx
    processmgrplugin.cxx
    procfs.cxx
    procfs.hxx
)

add_compile_definitions(ER_PROCESSMGR_EXPORTS=1)

set_property(TARGET ${EREBUS_PROCESSMGR} PROPERTY PREFIX "")

target_precompile_headers(${EREBUS_PROCESSMGR} PRIVATE ../../include/erebus/erebus.hxx)

target_link_libraries(${EREBUS_PROCESSMGR} PRIVATE ${EREBUS_RTLLIB}) 
