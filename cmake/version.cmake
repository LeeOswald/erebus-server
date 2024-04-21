function(get_version_from_file VER_FILE_NAME)
    file(READ ${VER_FILE_NAME} ER_VERSION_RAW)
    # Remove trailing whitespaces and/or newline
    string(STRIP ${ER_VERSION_RAW} ER_VERSION_)
    set(ER_VERSION ${ER_VERSION_} CACHE STRING
        "Project version determined from version file" FORCE
    )
    message(STATUS "Determined project version ${ER_VERSION}")
endfunction()