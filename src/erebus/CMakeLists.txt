if(ER_POSIX)
    set(PLATFORM_FILES
        ../../include/erebus/util/posixerror.hxx
        util/posixerror_posix.cxx
    )
elseif(ER_WINDOWS)
    set(PLATFORM_FILES
        ../../include/erebus/util/utf16.hxx
        ../../include/erebus/util/win32error.hxx
        util/utf16_win32.cxx
        util/win32error_win32.cxx
    )
endif()

add_library(${EREBUS_LIB} SHARED
    ../../include/erebus/empty.hxx
    ../../include/erebus/erebus.hxx
    ../../include/erebus/exception.hxx
    ../../include/erebus/knownprops.hxx
    ../../include/erebus/log.hxx
    ../../include/erebus/platform.hxx
    ../../include/erebus/property.hxx
    ../../include/erebus/sourcelocation.hxx
    ../../include/erebus/stringliteral.hxx
    ../../include/erebus/syncstream.hxx
    ../../include/erebus/system/process.hxx
    ../../include/erebus/system/time.hxx
    ../../include/erebus/util/autoptr.hxx
    ../../include/erebus/util/condition.hxx
    ../../include/erebus/util/crc32.hxx
    ../../include/erebus/util/exceptionutil.hxx
    ../../include/erebus/util/format.hxx
    ../../include/erebus/util/generichandle.hxx
    exception.cxx
    init.cxx
    knownprops.cxx
    log.cxx
    system/process.cxx
    system/time.cxx
    util/condition.cxx
    util/exceptionutil.cxx
    util/format.cxx
    ${PLATFORM_FILES}
)


add_compile_definitions(EREBUS_EXPORTS=1)

target_compile_features(${EREBUS_LIB} PUBLIC ${ER_CXX_FEATURES})
