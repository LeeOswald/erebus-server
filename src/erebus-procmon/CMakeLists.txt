

bpf_object(process process.bpf.c)
add_dependencies(process_skel libbpf-build bpftool-build generate_vmlinux_h)


add_library(${EREBUS_PROCMON} SHARED
    process_spy.cxx
    process_spy.hxx
    procmon.cxx
    procmon.hxx
)

add_compile_definitions(ER_PROCMON_EXPORTS=1)

target_compile_features(${EREBUS_PROCMON} PUBLIC ${ER_CXX_FEATURES})

target_link_libraries(${EREBUS_PROCMON} PRIVATE process_skel ${EREBUS_LIB} ${PLATFORM_LIBS}) 

add_subdirectory(tests)