if(ER_POSIX)
    set(PLATFORM_SOURCES
        ../../include/erebus/util/signalhandler.hxx
        system/user_linux.cxx
        util/signalhandler_posix.cxx
    )
elseif(ER_WINDOWS)
    set(PLATFORM_SOURCES
        ../../include/erebus/util/utf16.hxx
        ../../include/erebus/util/win32error.hxx
        system/user_win32.cxx
        util/utf16_win32.cxx
        util/win32error_win32.cxx
    )
endif()

if(ER_LINUX)
    set(PLATFORM_LIBS pthread dl backtrace)
elseif(ER_WINDOWS)
    set(PLATFORM_LIBS secur32)
endif()

add_library(${EREBUS_RTLLIB} SHARED
    ../../include/erebus/assert.hxx
    ../../include/erebus/condition.hxx
    ../../include/erebus/debug.hxx
    ../../include/erebus/empty.hxx
    ../../include/erebus/erebus.hxx
    ../../include/erebus/errno.hxx
    ../../include/erebus/exception.hxx
    ../../include/erebus/flags.hxx
    ../../include/erebus/idisposable.hxx
    ../../include/erebus/knownprops.hxx
    ../../include/erebus/location.hxx
    ../../include/erebus/lockable.hxx
    ../../include/erebus/log.hxx
    ../../include/erebus/lrucache.hxx
    ../../include/erebus/mutexpool.hxx
    ../../include/erebus/noncopyable.hxx
    ../../include/erebus/platform.hxx
    ../../include/erebus/property.hxx
    ../../include/erebus/propertybag.hxx
    ../../include/erebus/result.hxx
    ../../include/erebus/sourcelocation.hxx
    ../../include/erebus/stacktrace.hxx
    ../../include/erebus/stringliteral.hxx
    ../../include/erebus/syncstream.hxx
    ../../include/erebus/system/process.hxx
    ../../include/erebus/system/thread.hxx
    ../../include/erebus/system/time.hxx
    ../../include/erebus/system/timestamp.hxx
    ../../include/erebus/system/user.hxx
    ../../include/erebus/trackable.hxx
    ../../include/erebus/tree.hxx
    ../../include/erebus/util/autoptr.hxx
    ../../include/erebus/util/crc32.hxx
    ../../include/erebus/util/exceptionutil.hxx
    ../../include/erebus/util/file.hxx
    ../../include/erebus/util/format.hxx
    ../../include/erebus/util/generichandle.hxx
    ../../include/erebus/util/inifile.hxx
    ../../include/erebus/util/lockfile.hxx
    ../../include/erebus/util/pidfile.hxx
    ../../include/erebus/util/posixerror.hxx
    ../../include/erebus/util/ptrutils.hxx
    ../../include/erebus/util/sha256.hxx
    ../../include/erebus/util/stringutil.hxx
    assert.cxx
    debug.cxx
    errno.cxx
    exception.cxx
    init.cxx
    knownprops.cxx
    log.cxx
    stacktrace.cxx
    system/process.cxx
    system/thread.cxx
    system/time.cxx
    system/timestamp.cxx
    util/exceptionutil.cxx
    util/file.cxx
    util/format.cxx
    util/inifile.cxx
    util/lockfile.cxx
    util/posixerror.cxx
    util/sha256.cxx
    ${PLATFORM_SOURCES}
)


add_compile_definitions(EREBUS_EXPORTS=1)

set_property(TARGET ${EREBUS_RTLLIB} PROPERTY PREFIX "")

target_link_libraries(${EREBUS_RTLLIB} PRIVATE ${Boost_LIBRARIES} ${PLATFORM_LIBS})
