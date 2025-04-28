set(TARGET_NAME erebus-test-lib)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
    PRIVATE
        test_application.cxx
        test_event_listener.cxx
    PUBLIC
        FILE_SET headers TYPE HEADERS
            BASE_DIRS ${ER_INCLUDE_DIR}/testing
            FILES
                ${ER_INCLUDE_DIR}/testing/test_application.hxx
                ${ER_INCLUDE_DIR}/testing/test_event_listener.hxx
    
)

target_link_libraries(${TARGET_NAME} PUBLIC gtest::gtest erebus::rtl_lib)