set(TARGET_NAME erebus-proctree)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
    PRIVATE
        linux/procfs.cxx
        plugin.cxx
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
    ${TARGET_NAME}
    PUBLIC
    erebus::rtl_lib
    fmt::fmt
    ${Boost_LIBRARIES}
)

target_compile_definitions(${TARGET_NAME} PRIVATE ER_PROCTREE_EXPORTS)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

add_subdirectory(tests)
