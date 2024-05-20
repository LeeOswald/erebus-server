if(ER_USE_SYSTEM_GRPC)

    if(NOT MSVC)
        find_package(utf8_range REQUIRED)
    endif()
    find_package(Protobuf REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    include_directories("${gRPC_DIR}/../../../include") 

    set(GRPC_LIBS
        gRPC::grpc++ 
        gRPC::grpc++_reflection 
        gRPC::gpr 
        gRPC::grpc 
        protobuf::libprotoc 
        protobuf::libprotobuf 
        absl::flags 
        absl::flags_parse 
        absl::log absl::log_internal_check_op 
        utf8_range::utf8_validity    
    )

else()
    set(protobuf_INSTALL OFF)
    set(utf8_range_ENABLE_INSTALL OFF)

    FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG        v1.63.0
    )

    FetchContent_GetProperties(gRPC)

    if(NOT grpc_POPULATED)
        FetchContent_Populate(gRPC)
        add_subdirectory("${grpc_SOURCE_DIR}" "${grpc_BINARY_DIR}")
        
        include_directories("${absl_SOURCE_DIR}" "${grpc_SOURCE_DIR}/include" "${protobuf_SOURCE_DIR}/src")
    endif()


    set(GRPC_LIBS
        grpc++ 
        grpc++_reflection 
        gpr 
        grpc 
        libprotoc 
        libprotobuf 
        absl::flags
        absl::flags_parse 
        absl::log
        absl::log_internal_message
        absl::log_internal_check_op 
        utf8_validity
    )

endif()


