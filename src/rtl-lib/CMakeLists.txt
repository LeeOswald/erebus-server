set(TARGET_NAME erebus-rtl-lib)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
    PRIVATE
        assert.cxx
        debug.cxx
        error.cxx
        exception.cxx
        logger/logger.cxx
        logger/logger_base.hxx
        logger/async_logger.cxx
        logger/file_sink.cxx
        logger/null_logger.hxx
        logger/ostream_sink.cxx
        logger/simple_filter.cxx
        logger/simple_formatter.cxx
        logger/sink_base.hxx
        logger/sync_logger.cxx
        logger/tee.cxx
        luaxx.cxx
        luaxx/luaxx_base_fun.cxx
        luaxx/luaxx_exception_handler.cxx
        luaxx/luaxx_int64.cxx
        luaxx/luaxx_metatable_registry.cxx
        luaxx/luaxx_selector.cxx
        luaxx/luaxx_state.cxx
        luaxx/luaxx_util.cxx
        program.cxx
        property.cxx
        property_bag.cxx
        property_format.cxx
        stack_trace.cxx
        system/process.cxx
        system/thread.cxx
        time.cxx
        type_id.cxx
        util/exception_util.cxx
        util/file.cxx
        util/lock_file.cxx
        
    PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR} 
            FILES
                ${ER_INCLUDE_DIR}/iunknown.hxx
                ${ER_INCLUDE_DIR}/platform.hxx
                ${ER_INCLUDE_DIR}/rtl/assert.hxx
                ${ER_INCLUDE_DIR}/rtl/binary.hxx
                ${ER_INCLUDE_DIR}/rtl/bool.hxx
                ${ER_INCLUDE_DIR}/rtl/debug.hxx
                ${ER_INCLUDE_DIR}/rtl/empty.hxx
                ${ER_INCLUDE_DIR}/rtl/condition.hxx
                ${ER_INCLUDE_DIR}/rtl/error.hxx
                ${ER_INCLUDE_DIR}/rtl/exception.hxx
                ${ER_INCLUDE_DIR}/rtl/flags.hxx
                ${ER_INCLUDE_DIR}/rtl/format.hxx
                ${ER_INCLUDE_DIR}/rtl/log.hxx
                ${ER_INCLUDE_DIR}/rtl/logger/file_sink.hxx
                ${ER_INCLUDE_DIR}/rtl/logger/ostream_sink.hxx
                ${ER_INCLUDE_DIR}/rtl/logger/simple_filter.hxx
                ${ER_INCLUDE_DIR}/rtl/logger/simple_formatter.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_base_fun.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_class.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_class_fun.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_ctor.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_dtor.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_exception_handler.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_exception_types.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_fun.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_function.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_int64.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_lua_name.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_lua_ref.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_metatable_registry.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_obj.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_obj_fun.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_primitives.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_references.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_registry.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_resource_handler.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_selector.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_state.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_traits.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_tuple.hxx
                ${ER_INCLUDE_DIR}/rtl/luaxx/luaxx_util.hxx
                ${ER_INCLUDE_DIR}/rtl/mpl.hxx
                ${ER_INCLUDE_DIR}/rtl/multi_string.hxx
                ${ER_INCLUDE_DIR}/rtl/rtl.hxx
                ${ER_INCLUDE_DIR}/rtl/program.hxx
                ${ER_INCLUDE_DIR}/rtl/property_format.hxx
                ${ER_INCLUDE_DIR}/rtl/property.hxx
                ${ER_INCLUDE_DIR}/rtl/property_bag.hxx
                ${ER_INCLUDE_DIR}/rtl/reflectable.hxx
                ${ER_INCLUDE_DIR}/rtl/stack_trace.hxx
                ${ER_INCLUDE_DIR}/rtl/string_literal.hxx
                ${ER_INCLUDE_DIR}/rtl/system/process.hxx
                ${ER_INCLUDE_DIR}/rtl/system/thread.hxx
                ${ER_INCLUDE_DIR}/rtl/system/user.hxx
                ${ER_INCLUDE_DIR}/rtl/time.hxx
                ${ER_INCLUDE_DIR}/rtl/type_id.hxx
                ${ER_INCLUDE_DIR}/rtl/util/auto_ptr.hxx
                ${ER_INCLUDE_DIR}/rtl/util/errno.hxx
                ${ER_INCLUDE_DIR}/rtl/util/exception_util.hxx
                ${ER_INCLUDE_DIR}/rtl/util/file.hxx
                ${ER_INCLUDE_DIR}/rtl/util/generic_handle.hxx
                ${ER_INCLUDE_DIR}/rtl/util/lock_file.hxx
                ${ER_INCLUDE_DIR}/rtl/util/null_mutex.hxx
                ${ER_INCLUDE_DIR}/rtl/util/pattern.hxx
                ${ER_INCLUDE_DIR}/rtl/util/pid_file.hxx
                ${ER_INCLUDE_DIR}/rtl/util/string_util.hxx
                ${ER_INCLUDE_DIR}/rtl/util/thread_data.hxx
                ${ER_INCLUDE_DIR}/rtl/util/unknown_base.hxx
                ${ER_INCLUDE_DIR}/rtl/util/utf16.hxx
)

if(ER_WINDOWS)
    target_sources(${TARGET_NAME}
        PRIVATE
            logger/win32_debugger_sink.cxx
            system/user_win32.cxx
            util/win32_file.cxx
            util/win32_utf16.cxx
        PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR}
            FILES
                ${ER_INCLUDE_DIR}/rtl/logger/win32_debugger_sink.hxx
    )
        
elseif(ER_POSIX)
    target_sources(${TARGET_NAME}
        PRIVATE
            logger/syslog_linux_sink.cxx
            system/user_linux.cxx
            util/posix_file.cxx
        PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS 
                ${ER_INCLUDE_DIR}
            FILES
                ${ER_INCLUDE_DIR}/rtl/logger/syslog_linux_sink.hxx
    )
    
endif()


if(ER_LINUX)
    set(PLATFORM_LIBS pthread dl backtrace)
elseif(ER_WINDOWS)
    set(PLATFORM_LIBS secur32)
endif()

target_link_libraries(
    ${TARGET_NAME}
    PUBLIC
    fmt::fmt
    rapidjson
    ${Boost_LIBRARIES}
    ${lua_LIBRARIES}
    ${PLATFORM_LIBS}
)

target_compile_definitions(${TARGET_NAME} PRIVATE ER_RTL_EXPORTS)

set_property(TARGET ${TARGET_NAME} PROPERTY PREFIX "")

if(NOT ER_BUILD_CLIENT_LIBS_ONLY)
    add_subdirectory(tests)
endif()
