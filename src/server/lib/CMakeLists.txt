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

add_library(erebus-server-lib SHARED)

target_sources(erebus-server-lib
	PRIVATE
        ${PLATFORM_SOURCES}

    PUBLIC
		FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/server/server_lib.hxx
                ${ER_INCLUDE_DIR}/server/system_info.hxx
)

target_link_libraries(erebus-server-lib PUBLIC erebus::rtl fmt::fmt ${Boost_LIBRARIES} ${PLATFORM_LIBS})

target_compile_definitions(erebus-server-lib PRIVATE ER_SERVER_EXPORTS)

set_property(TARGET erebus-server-lib PROPERTY PREFIX "")