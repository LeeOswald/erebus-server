
add_executable(erebus-rtl-tests)

target_sources(erebus-rtl-tests
    PRIVATE
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
        property.cpp
        property_bag.cpp
    PRIVATE
        FILE_SET private_headers TYPE HEADERS
            FILES
                common.hpp
)

target_link_libraries(erebus-rtl-tests PRIVATE erebus::testing erebus::rtl)

add_test(NAME erebus-rtl COMMAND erebus-rtl-tests)


