add_library(${EREBUS_PROCESSMGR} SHARED
    ../../include/erebus-processmgr/desktopentry.hxx
    ../../include/erebus-processmgr/iconcache.hxx
    ../../include/erebus-processmgr/pathresolver.hxx
    ../../include/erebus-processmgr/processmgr.hxx
    ../../include/erebus-processmgr/processprops.hxx
    ../../include/erebus-processmgr/procfs.hxx
    desktopentry.cxx
    iconcache.cxx
    pathresolver.cxx
    processlist.cxx
    processlist.hxx
    processlistdiff.cxx
    processlistdiff.hxx
    processmgr.cxx
    procfs.cxx
)

add_compile_definitions(ER_PROCESSMGR_EXPORTS=1)

target_compile_features(${EREBUS_LIB} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_PROCESSMGR} PRIVATE ${EREBUS_LIB} ${PLATFORM_LIBS}) 
