add_library(erebus-proctree SHARED)

target_sources(erebus-proctree
    PRIVATE
        linux/procfs.cxx
        process_props.cxx
        
    PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/proctree/linux/procfs.hxx
                ${ER_INCLUDE_DIR}/proctree/process_props.hxx
                ${ER_INCLUDE_DIR}/proctree/proctree.hxx
)


target_link_libraries(
    erebus-proctree
    PUBLIC
    fmt::fmt
    ${Boost_LIBRARIES}
)

target_compile_definitions(erebus-proctree PRIVATE ER_PROCTREE_EXPORTS)

set_property(TARGET erebus-proctree PROPERTY PREFIX "")

add_subdirectory(tests)
