

bpf_object(process process.bpf.c)
add_dependencies(process_skel libbpf-build bpftool-build generate_vmlinux_h)


add_library(${EREBUS_PROCMON} SHARED
    process_spy.cxx
    process_spy.hxx
    procmon.cxx
    procmon.hxx
)

add_compile_definitions(ER_PROCMON_EXPORTS=1)

set_property(TARGET ${EREBUS_PROCMON} PROPERTY PREFIX "")

target_precompile_headers(${EREBUS_PROCMON} PRIVATE ../../include/erebus/erebus.hxx)

target_link_libraries(${EREBUS_PROCMON} PRIVATE process_skel ${EREBUS_RTLLIB}) 

add_subdirectory(tests)
