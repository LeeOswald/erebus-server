add_library(erebus-testing STATIC)

target_sources(erebus-testing
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

target_link_libraries(erebus-testing PUBLIC gtest::gtest erebus::rtl)