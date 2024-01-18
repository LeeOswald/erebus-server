add_library(${EREBUS_LIB} SHARED
    ../../include/erebus/empty.hxx
    ../../include/erebus/erebus.hxx
    ../../include/erebus/platform.hxx
    init.cxx
    ${PLATFORM_FILES}
)


add_compile_definitions(EREBUS_EXPORTS=1)

target_compile_features(${EREBUS_LIB} PUBLIC ${ER_CXX_FEATURES})
