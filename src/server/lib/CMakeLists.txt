set(TARGET_NAME erebus-server-lib)

if(ER_WINDOWS)

    set(PLATFORM_SOURCES
        win32_system_info.cxx
    )

    set(PLATFORM_LIBS ntdll)

elseif(ER_LINUX)
    
    set(PLATFORM_SOURCES
        linux_system_info.cxx
    )

endif()

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
	PRIVATE
        plugin_mgr.cxx
        system_info_common.hxx
        system_info_common.cxx
        ${PLATFORM_SOURCES}

    PUBLIC
		FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/server/iplugin.hxx
                ${ER_INCLUDE_DIR}/server/iplugin_host.hxx
                ${ER_INCLUDE_DIR}/server/plugin_mgr.hxx
                ${ER_INCLUDE_DIR}/server/server_lib.hxx
                ${ER_INCLUDE_DIR}/server/system_info.hxx
)

target_link_libraries(${TARGET_NAME} PUBLIC erebus::rtl_lib fmt::fmt ${Boost_LIBRARIES} ${PLATFORM_LIBS})

target_compile_definitions(${TARGET_NAME} PRIVATE ER_SERVER_EXPORTS)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")