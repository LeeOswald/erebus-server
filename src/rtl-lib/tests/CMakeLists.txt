set(TARGET_NAME erebus-rtl-tests)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE
        error.cpp
        flags.cpp
        luaxx_class.cpp
        luaxx_error.cpp
        luaxx_exception.cpp
        luaxx_int64.cpp
        luaxx_interop.cpp
        luaxx_metatable.cpp
        luaxx_obj.cpp
        luaxx_reference.cpp
        luaxx_selector.cpp
        main.cpp
        multi_string.cpp
        pattern.cpp
        property.cpp
        property_bag.cpp
        string_util.cpp
        reflectable.cpp
    PRIVATE
        FILE_SET private_headers TYPE HEADERS
            FILES
                common.hpp
)

target_link_libraries(${TARGET_NAME} PRIVATE erebus::test_lib erebus::rtl_lib)

add_test(NAME erebus-rtl-tests COMMAND ${TARGET_NAME})


